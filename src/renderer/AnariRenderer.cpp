#include "AnariRenderer.h"

#include <anari/anari.h>

#include <QByteArray>
#include <QColor>
#include <QDebug>
#include <QTimer>

#include <cstring>
#include <memory>

#include "DataLoaderFactory.h"
#include "PerspectiveCamera.h"

namespace vitrine
{
AnariRenderer::AnariRenderer(QObject* parent)
    : QObject(parent)
    , m_camera(std::make_unique<PerspectiveCamera>())
{
    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(kRenderIntervalMs);
    m_renderTimer->setTimerType(Qt::PreciseTimer);
    connect(m_renderTimer, &QTimer::timeout, this, &AnariRenderer::renderTick);

    // Pre-allocate a 1x1 placeholder so paintEvent has something valid even
    // before the first real frame lands.
    m_frameImage = QImage(1, 1, QImage::Format_RGBA8888);
    m_frameImage.fill(Qt::black);
}

AnariRenderer::~AnariRenderer()
{
    destroyBackend();
}

void AnariRenderer::emitStatus(LogLevel level, const QString& message)
{
    emit statusMessage(static_cast<int>(level), message);
}

void AnariRenderer::destroyBackend()
{
    // Drain any in-flight frame before tearing the device down: releasing
    // ANARI objects while the backend is still touching them is the
    // canonical recipe for embree/CUDA/Vulkan crashes.
    if (m_device && m_anariScene.frame && m_frameInFlight) {
        anariFrameReady(m_device, m_anariScene.frame, ANARI_WAIT);
        m_frameInFlight = false;
    }
    if (m_device) {
        m_anariScene.releaseSceneObjects(m_device);
        anariRelease(m_device, m_device);
        m_device = nullptr;
    }
    if (m_library) {
        anariUnloadLibrary(m_library);
        m_library = nullptr;
    }
    m_statusSink.reset();
}

void AnariRenderer::loadBackend(const QString& libraryName, const QString& deviceSubtype)
{
    if (m_renderTimer->isActive()) {
        m_renderTimer->stop();
    }
    destroyBackend();

    m_libraryName = libraryName;
    m_deviceSubtype = deviceSubtype.isEmpty() ? QStringLiteral("default") : deviceSubtype;

    m_statusSink = std::make_unique<AnariStatusSink>(
        [this](LogLevel level, QString msg) {
            // statusMessage is a queued connection from the renderer thread
            // back to the UI; no Qt::QueuedConnection needed here because the
            // signal->slot connection itself enforces the thread crossing.
            emit statusMessage(static_cast<int>(level), msg);
        });

    const QByteArray libUtf8 = libraryName.toUtf8();
    m_library = anariLoadLibrary(libUtf8.constData(),
                                 m_statusSink->callback(),
                                 m_statusSink->userData());
    if (!m_library) {
        emitStatus(LogLevel::Error,
                   QStringLiteral("anariLoadLibrary(\"%1\") failed.").arg(libraryName));
        emit backendLoaded(false, libraryName, m_deviceSubtype);
        m_statusSink.reset();
        return;
    }

    const QByteArray subtypeUtf8 = m_deviceSubtype.toUtf8();
    m_device = anariNewDevice(m_library, subtypeUtf8.constData());
    if (!m_device) {
        emitStatus(LogLevel::Error,
                   QStringLiteral("anariNewDevice(\"%1\") failed.").arg(m_deviceSubtype));
        anariUnloadLibrary(m_library);
        m_library = nullptr;
        m_statusSink.reset();
        emit backendLoaded(false, libraryName, m_deviceSubtype);
        return;
    }
    anariCommitParameters(m_device, m_device);

    const QByteArray rendererUtf8 = m_rendererSubtype.toUtf8();
    m_anariScene.renderer = anariNewRenderer(m_device, rendererUtf8.constData());
    if (!m_anariScene.renderer) {
        emitStatus(LogLevel::Error,
                   QStringLiteral("anariNewRenderer(\"%1\") returned null.")
                       .arg(m_rendererSubtype));
    } else {
        anariCommitParameters(m_device, m_anariScene.renderer);
    }

    m_anariScene.camera = anariNewCamera(m_device, m_camera->anariSubtype());
    if (m_anariScene.camera) {
        // The camera holds the authoritative view state (default: eye at
        // +Z looking at the origin). commitCamera() pushes it and notifies
        // the overlay.
        m_camera->setAspect(static_cast<float>(m_size.width()) /
                            static_cast<float>(std::max(1, m_size.height())));
        commitCamera();
    }

    m_anariScene.world = anariNewWorld(m_device);
    if (m_anariScene.world) {
        anariCommitParameters(m_device, m_anariScene.world);
    }

    rebuildFrame();

    emitStatus(LogLevel::Info,
               QStringLiteral("Loaded ANARI backend \"%1\" (device \"%2\").")
                   .arg(libraryName, m_deviceSubtype));
    emit backendLoaded(true, libraryName, m_deviceSubtype);
}

void AnariRenderer::setRendererSubtype(const QString& subtype)
{
    if (subtype.isEmpty() || subtype == m_rendererSubtype) {
        return;
    }
    m_rendererSubtype = subtype;
    if (!m_device) {
        return;
    }
    if (m_anariScene.renderer) {
        anariRelease(m_device, m_anariScene.renderer);
        m_anariScene.renderer = nullptr;
    }
    const QByteArray subtypeUtf8 = subtype.toUtf8();
    m_anariScene.renderer = anariNewRenderer(m_device, subtypeUtf8.constData());
    if (m_anariScene.renderer) {
        anariCommitParameters(m_device, m_anariScene.renderer);
        if (m_anariScene.frame) {
            anariSetParameter(m_device, m_anariScene.frame, "renderer", ANARI_RENDERER, &m_anariScene.renderer);
            anariCommitParameters(m_device, m_anariScene.frame);
        }
        emitStatus(LogLevel::Info,
                   QStringLiteral("Renderer subtype set to \"%1\".").arg(subtype));
    } else {
        emitStatus(LogLevel::Warning,
                   QStringLiteral("Renderer subtype \"%1\" not supported by backend.")
                       .arg(subtype));
    }
}

void AnariRenderer::setRendererParameter(const QString& name, int typeInt, const QVariant& value)
{
    if (!m_device || !m_anariScene.renderer || name.isEmpty()) {
        return;
    }
    ANARIRenderer renderer = m_anariScene.renderer;
    const auto type = static_cast<ANARIDataType>(typeInt);
    const QByteArray nameUtf8 = name.toUtf8();

    switch (type) {
        case ANARI_FLOAT32: {
            const float f = static_cast<float>(value.toDouble());
            anariSetParameter(m_device, renderer, nameUtf8.constData(), ANARI_FLOAT32, &f);
            break;
        }
        case ANARI_INT32: {
            const int32_t v = value.toInt();
            anariSetParameter(m_device, renderer, nameUtf8.constData(), ANARI_INT32, &v);
            break;
        }
        case ANARI_UINT32: {
            const uint32_t v = static_cast<uint32_t>(value.toUInt());
            anariSetParameter(m_device, renderer, nameUtf8.constData(), ANARI_UINT32, &v);
            break;
        }
        case ANARI_BOOL: {
            const int v = value.toBool() ? 1 : 0;
            anariSetParameter(m_device, renderer, nameUtf8.constData(), ANARI_BOOL, &v);
            break;
        }
        case ANARI_FLOAT32_VEC4: {
            std::array<float, 4> v{};
            if (unwrapVariantToFloat32Vec4(value, v).toBool()) {
                anariSetParameter(m_device, renderer, nameUtf8.constData(),
                                  ANARI_FLOAT32_VEC4, v.data());
            }
            break;
        }
        case ANARI_STRING: {
            const QByteArray s = value.toString().toUtf8();
            anariSetParameter(m_device, renderer, nameUtf8.constData(),
                              ANARI_STRING, s.constData());
            break;
        }
        default:
            emitStatus(LogLevel::Warning,
                       QStringLiteral("Renderer parameter \"%1\": unhandled ANARI type %2.")
                           .arg(name).arg(typeInt));
            return;
    }
    anariCommitParameters(m_device, renderer);
}

void AnariRenderer::resize(const QSize& size)
{
    const QSize clamped(std::max(1, size.width()), std::max(1, size.height()));
    if (clamped == m_size && !m_frameImage.isNull()) {
        return;
    }
    m_size = clamped;
    m_frameImage = QImage(m_size, QImage::Format_RGBA8888);
    m_frameImage.fill(Qt::black);

    if (m_device && m_anariScene.camera) {
        m_camera->setAspect(static_cast<float>(m_size.width()) /
                            static_cast<float>(std::max(1, m_size.height())));
        commitCamera();
    }
    rebuildFrame();
}

void AnariRenderer::rebuildFrame()
{
    if (!m_device) {
        return;
    }
    if (!m_anariScene.frame) {
        m_anariScene.frame = anariNewFrame(m_device);
    }
    if (!m_anariScene.frame) {
        return;
    }
    ANARIFrame frame = m_anariScene.frame;
    // Drain any in-flight frame before re-parameterising; otherwise the
    // backend may still be reading the old size/camera/world while we
    // overwrite them.
    if (m_frameInFlight) {
        anariFrameReady(m_device, frame, ANARI_WAIT);
        m_frameInFlight = false;
    }
    const uint32_t size[2] = {static_cast<uint32_t>(m_size.width()),
                              static_cast<uint32_t>(m_size.height())};
    ANARIDataType colorType = ANARI_UFIXED8_VEC4;
    anariSetParameter(m_device, frame, "size", ANARI_UINT32_VEC2, size);
    anariSetParameter(m_device, frame, "channel.color", ANARI_DATA_TYPE, &colorType);
    if (m_anariScene.renderer) {
        anariSetParameter(m_device, frame, "renderer", ANARI_RENDERER, &m_anariScene.renderer);
    }
    if (m_anariScene.camera) {
        anariSetParameter(m_device, frame, "camera", ANARI_CAMERA, &m_anariScene.camera);
    }
    if (m_anariScene.world) {
        anariSetParameter(m_device, frame, "world", ANARI_WORLD, &m_anariScene.world);
    }
    anariCommitParameters(m_device, frame);
}

void AnariRenderer::loadSceneFromFile(const QString& path)
{
    if (!m_device || !m_anariScene.world) {
        return;
    }

    auto loader = DataLoaderFactory::createLoader(path);
    if (!loader) {
        emitStatus(LogLevel::Warning,
                   QStringLiteral("Unsupported file type: \"%1\".").arg(path));
        return;
    }
    // Chain the loader's status messages straight into ours so they reach the
    // application log. The connection dies with the loader at end of scope.
    connect(loader.get(), &DataLoader::statusMessage,
            this, &AnariRenderer::statusMessage);

    // Drain any in-flight frame and release prior scene content (surfaces /
    // groups / instances) before the loader rebuilds it; the frame state lives
    // here, so the renderer owns this reset rather than the loader.
    if (m_anariScene.frame && m_frameInFlight) {
        anariFrameReady(m_device, m_anariScene.frame, ANARI_WAIT);
        m_frameInFlight = false;
    }
    m_anariScene.releaseContent(m_device);

    loader->loadSceneFromFile(m_device, m_anariScene, path);
}

void AnariRenderer::commitCamera()
{
    if (!m_device || !m_anariScene.camera || !m_camera) {
        return;
    }
    m_camera->commit(m_device, m_anariScene.camera);
    emit cameraChanged(m_camera->right(), m_camera->upVector(), m_camera->forward());
}

void AnariRenderer::orbitCamera(const QPointF& deltaPixels)
{
    if (!m_camera) {
        return;
    }
    // Drag right -> orbit left (yaw), drag down -> orbit down (pitch).
    m_camera->orbit(static_cast<float>(-deltaPixels.x()) * kOrbitRadiansPerPixel,
                    static_cast<float>(deltaPixels.y()) * kOrbitRadiansPerPixel);
    commitCamera();
}

void AnariRenderer::panCamera(const QPointF& deltaPixels)
{
    if (!m_camera) {
        return;
    }
    const float scale = m_camera->distance() * kPanFractionPerPixel;
    m_camera->pan(static_cast<float>(deltaPixels.x()) * scale,
                  static_cast<float>(deltaPixels.y()) * scale);
    commitCamera();
}

void AnariRenderer::dollyCamera(float steps)
{
    if (!m_camera) {
        return;
    }
    m_camera->dolly(steps * m_camera->distance() * kDollyFractionPerStep);
    commitCamera();
}

CameraConfig AnariRenderer::cameraConfig() const
{
    return m_camera ? m_camera->toConfig() : CameraConfig{};
}

void AnariRenderer::setCameraConfig(const CameraConfig& config)
{
    if (!m_camera) {
        return;
    }

    const bool subtypeChanged = config.type != m_camera->type();

    if (subtypeChanged) {
        // Swap the Camera object so applyConfig targets the right subtype.
        m_camera = makeCamera(config.type);

        if (m_device) {
            // The ANARI camera subtype is fixed at creation, so a type change
            // means a new handle. Drain any in-flight frame before releasing
            // the old one — the backend may still be reading it.
            if (m_anariScene.camera && m_frameInFlight && m_anariScene.frame) {
                anariFrameReady(m_device, m_anariScene.frame, ANARI_WAIT);
                m_frameInFlight = false;
            }
            if (m_anariScene.camera) {
                anariRelease(m_device, m_anariScene.camera);
                m_anariScene.camera = nullptr;
            }
            m_anariScene.camera = anariNewCamera(m_device, m_camera->anariSubtype());
        }
    }

    m_camera->setAspect(static_cast<float>(m_size.width()) /
                        static_cast<float>(std::max(1, m_size.height())));
    m_camera->applyConfig(config);

    if (subtypeChanged) {
        // Re-point the frame at the new camera handle, then commit state.
        rebuildFrame();
    }
    commitCamera();

    emitStatus(LogLevel::Info,
               QStringLiteral("Camera set to \"%1\".").arg(m_camera->anariSubtype()));
}

void AnariRenderer::start()
{
    if (!m_renderTimer->isActive()) {
        m_renderTimer->start();
    }
}

void AnariRenderer::stop()
{
    if (m_renderTimer->isActive()) {
        m_renderTimer->stop();
    }
}

void AnariRenderer::renderTick()
{
    if (!m_device || !m_anariScene.frame) {
        return;
    }
    ANARIFrame frame = m_anariScene.frame;

    // Two-state pump: kick a render on one tick, poll for completion on
    // subsequent ticks via ANARI_NO_WAIT. This keeps the GUI thread free
    // while the backend's own worker threads do the actual rendering.
    if (!m_frameInFlight) {
        anariRenderFrame(m_device, frame);
        m_frameInFlight = true;
        return;
    }

    if (anariFrameReady(m_device, frame, ANARI_NO_WAIT) != 1) {
        return;
    }

    uint32_t w = 0;
    uint32_t h = 0;
    ANARIDataType pixelType = ANARI_UNKNOWN;
    const void* pixels =
        anariMapFrame(m_device, frame, "channel.color", &w, &h, &pixelType);
    m_frameInFlight = false;

    if (!pixels || pixelType != ANARI_UFIXED8_VEC4) {
        if (pixels) {
            anariUnmapFrame(m_device, frame, "channel.color");
        }
        return;
    }

    if (w != static_cast<uint32_t>(m_frameImage.width()) ||
        h != static_cast<uint32_t>(m_frameImage.height())) {
        m_frameImage = QImage(static_cast<int>(w), static_cast<int>(h),
                              QImage::Format_RGBA8888);
    }

    std::memcpy(m_frameImage.bits(), pixels, static_cast<size_t>(w) * h * 4u);
    anariUnmapFrame(m_device, frame, "channel.color");

    // QImage uses implicit sharing, so emitting by value is cheap; receiver
    // gets its own snapshot if it later modifies it.
    emit frameReady(m_frameImage);
}

// Static Functions
// ================

QVariant AnariRenderer::unwrapVariantToFloat32Vec4(const QVariant& v, std::array<float, 4>& out)
{
    if (v.canConvert<QColor>()) {
        const QColor c = v.value<QColor>();
        out = {static_cast<float>(c.redF()),
               static_cast<float>(c.greenF()),
               static_cast<float>(c.blueF()),
               static_cast<float>(c.alphaF())};
        return QVariant::fromValue(true);
    }
    const auto list = v.toList();
    if (list.size() == 4) {
        for (int i = 0; i < 4; ++i) {
            out[i] = static_cast<float>(list[i].toDouble());
        }
        return QVariant::fromValue(true);
    }
    return QVariant::fromValue(false);
}

} // namespace vitrine
