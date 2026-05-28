#include "AnariRenderer.h"

#include <anari/anari.h>

#include <QByteArray>
#include <QColor>
#include <QDebug>
#include <QTimer>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <cstring>
#include <unordered_map>

#include "AppUtils.h"

namespace vitrine
{

namespace
{

// Render loop interval (ms). We're on the GUI thread, so this is a soft cap
// on how often we kick a new frame / poll for completion. 16ms targets 60Hz;
// the actual render happens in parallel on backend-internal threads.
constexpr int kRenderIntervalMs = 16;

QVariant unwrapVariantToFloat32Vec4(const QVariant& v, std::array<float, 4>& out)
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

} // namespace

AnariRenderer::AnariRenderer(QObject* parent)
    : QObject(parent)
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
    if (m_device && m_frame && m_frameInFlight) {
        anariFrameReady(m_device, m_frame, ANARI_WAIT);
        m_frameInFlight = false;
    }
    if (m_device) {
        auto release = [this](auto& obj) {
            if (obj) {
                anariRelease(m_device, obj);
                obj = nullptr;
            }
        };
        release(m_instance);
        release(m_group);
        release(m_surface);
        release(m_material);
        release(m_geometry);
        release(m_frame);
        release(m_world);
        release(m_camera);
        release(m_renderer);
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
    m_renderer = anariNewRenderer(m_device, rendererUtf8.constData());
    if (!m_renderer) {
        emitStatus(LogLevel::Error,
                   QStringLiteral("anariNewRenderer(\"%1\") returned null.")
                       .arg(m_rendererSubtype));
    } else {
        anariCommitParameters(m_device, m_renderer);
    }

    m_camera = anariNewCamera(m_device, "perspective");
    if (m_camera) {
        // Defaults: position at origin looking down -Z. The host can refine
        // these once camera control is wired into the widget.
        const std::array<float, 3> position{0.0f, 0.0f, 0.0f};
        const std::array<float, 3> direction{0.0f, 0.0f, -1.0f};
        const std::array<float, 3> up{0.0f, 1.0f, 0.0f};
        anariSetParameter(m_device, m_camera, "position", ANARI_FLOAT32_VEC3, position.data());
        anariSetParameter(m_device, m_camera, "direction", ANARI_FLOAT32_VEC3, direction.data());
        anariSetParameter(m_device, m_camera, "up", ANARI_FLOAT32_VEC3, up.data());
        const float aspect = static_cast<float>(m_size.width()) /
                             static_cast<float>(std::max(1, m_size.height()));
        anariSetParameter(m_device, m_camera, "aspect", ANARI_FLOAT32, &aspect);
        anariCommitParameters(m_device, m_camera);
    }

    m_world = anariNewWorld(m_device);
    if (m_world) {
        anariCommitParameters(m_device, m_world);
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
    if (m_renderer) {
        anariRelease(m_device, m_renderer);
        m_renderer = nullptr;
    }
    const QByteArray subtypeUtf8 = subtype.toUtf8();
    m_renderer = anariNewRenderer(m_device, subtypeUtf8.constData());
    if (m_renderer) {
        anariCommitParameters(m_device, m_renderer);
        if (m_frame) {
            anariSetParameter(m_device, m_frame, "renderer", ANARI_RENDERER, &m_renderer);
            anariCommitParameters(m_device, m_frame);
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
    if (!m_device || !m_renderer || name.isEmpty()) {
        return;
    }
    const auto type = static_cast<ANARIDataType>(typeInt);
    const QByteArray nameUtf8 = name.toUtf8();

    switch (type) {
        case ANARI_FLOAT32: {
            const float f = static_cast<float>(value.toDouble());
            anariSetParameter(m_device, m_renderer, nameUtf8.constData(), ANARI_FLOAT32, &f);
            break;
        }
        case ANARI_INT32: {
            const int32_t v = value.toInt();
            anariSetParameter(m_device, m_renderer, nameUtf8.constData(), ANARI_INT32, &v);
            break;
        }
        case ANARI_UINT32: {
            const uint32_t v = static_cast<uint32_t>(value.toUInt());
            anariSetParameter(m_device, m_renderer, nameUtf8.constData(), ANARI_UINT32, &v);
            break;
        }
        case ANARI_BOOL: {
            const int v = value.toBool() ? 1 : 0;
            anariSetParameter(m_device, m_renderer, nameUtf8.constData(), ANARI_BOOL, &v);
            break;
        }
        case ANARI_FLOAT32_VEC4: {
            std::array<float, 4> v{};
            if (unwrapVariantToFloat32Vec4(value, v).toBool()) {
                anariSetParameter(m_device, m_renderer, nameUtf8.constData(),
                                  ANARI_FLOAT32_VEC4, v.data());
            }
            break;
        }
        case ANARI_STRING: {
            const QByteArray s = value.toString().toUtf8();
            anariSetParameter(m_device, m_renderer, nameUtf8.constData(),
                              ANARI_STRING, s.constData());
            break;
        }
        default:
            emitStatus(LogLevel::Warning,
                       QStringLiteral("Renderer parameter \"%1\": unhandled ANARI type %2.")
                           .arg(name).arg(typeInt));
            return;
    }
    anariCommitParameters(m_device, m_renderer);
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

    if (m_device && m_camera) {
        const float aspect = static_cast<float>(m_size.width()) /
                             static_cast<float>(std::max(1, m_size.height()));
        anariSetParameter(m_device, m_camera, "aspect", ANARI_FLOAT32, &aspect);
        anariCommitParameters(m_device, m_camera);
    }
    rebuildFrame();
}

void AnariRenderer::rebuildFrame()
{
    if (!m_device) {
        return;
    }
    if (!m_frame) {
        m_frame = anariNewFrame(m_device);
    }
    if (!m_frame) {
        return;
    }
    // Drain any in-flight frame before re-parameterising; otherwise the
    // backend may still be reading the old size/camera/world while we
    // overwrite them.
    if (m_frameInFlight) {
        anariFrameReady(m_device, m_frame, ANARI_WAIT);
        m_frameInFlight = false;
    }
    const uint32_t size[2] = {static_cast<uint32_t>(m_size.width()),
                              static_cast<uint32_t>(m_size.height())};
    ANARIDataType colorType = ANARI_UFIXED8_VEC4;
    anariSetParameter(m_device, m_frame, "size", ANARI_UINT32_VEC2, size);
    anariSetParameter(m_device, m_frame, "channel.color", ANARI_DATA_TYPE, &colorType);
    if (m_renderer) {
        anariSetParameter(m_device, m_frame, "renderer", ANARI_RENDERER, &m_renderer);
    }
    if (m_camera) {
        anariSetParameter(m_device, m_frame, "camera", ANARI_CAMERA, &m_camera);
    }
    if (m_world) {
        anariSetParameter(m_device, m_frame, "world", ANARI_WORLD, &m_world);
    }
    anariCommitParameters(m_device, m_frame);
}

void AnariRenderer::setSceneFromObj(const QString& path)
{
    if (!m_device || !m_world) {
        return;
    }

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.toStdString().c_str())) {
        emitStatus(LogLevel::Warning,
                   QStringLiteral("Failed to load OBJ '%1': %2 %3")
                       .arg(path, QString::fromStdString(warn), QString::fromStdString(err)));
        return;
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices;
    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex v{};
            v.pos = {attrib.vertices[3 * index.vertex_index + 0],
                     attrib.vertices[3 * index.vertex_index + 1],
                     attrib.vertices[3 * index.vertex_index + 2]};
            if (index.texcoord_index >= 0) {
                v.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                              1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
            }
            v.color = {1.0f, 1.0f, 1.0f};
            auto it = uniqueVertices.find(v);
            if (it == uniqueVertices.end()) {
                const uint32_t newIdx = static_cast<uint32_t>(verts.size());
                uniqueVertices.emplace(v, newIdx);
                verts.push_back(v);
                idx.push_back(newIdx);
            } else {
                idx.push_back(it->second);
            }
        }
    }
    if (verts.empty() || idx.size() < 3) {
        emitStatus(LogLevel::Warning,
                   QStringLiteral("OBJ '%1' produced no triangles.").arg(path));
        return;
    }

    // Repack into plain float[3]/uint32_t[3] arrays the ANARI arrays will
    // reference directly. We keep the storage on this object so the
    // pointers stay valid for the lifetime of the geometry.
    m_positions.clear();
    m_positions.reserve(verts.size());
    for (const auto& v : verts) {
        m_positions.push_back({v.pos.x, v.pos.y, v.pos.z});
    }
    m_indices.clear();
    m_indices.reserve(idx.size() / 3);
    for (size_t i = 0; i + 2 < idx.size(); i += 3) {
        m_indices.push_back({idx[i], idx[i + 1], idx[i + 2]});
    }

    // Tear down any prior triangle geometry.
    auto release = [this](auto& obj) {
        if (obj) {
            anariRelease(m_device, obj);
            obj = nullptr;
        }
    };
    release(m_instance);
    release(m_group);
    release(m_surface);
    release(m_material);
    release(m_geometry);

    m_geometry = anariNewGeometry(m_device, "triangle");
    if (!m_geometry) {
        emitStatus(LogLevel::Warning,
                   QStringLiteral("Backend does not implement triangle geometry yet "
                                  "(anariNewGeometry returned null). Scene wiring is "
                                  "live; only clear-frame will render."));
        return;
    }

    ANARIArray1D positionArray = anariNewArray1D(
        m_device, m_positions.data(), nullptr, nullptr,
        ANARI_FLOAT32_VEC3, m_positions.size());
    ANARIArray1D indexArray = anariNewArray1D(
        m_device, m_indices.data(), nullptr, nullptr,
        ANARI_UINT32_VEC3, m_indices.size());
    anariSetParameter(m_device, m_geometry, "vertex.position", ANARI_ARRAY1D, &positionArray);
    anariSetParameter(m_device, m_geometry, "primitive.index", ANARI_ARRAY1D, &indexArray);
    anariRelease(m_device, positionArray);
    anariRelease(m_device, indexArray);
    anariCommitParameters(m_device, m_geometry);

    m_material = anariNewMaterial(m_device, "matte");
    if (m_material) {
        anariCommitParameters(m_device, m_material);
    }

    m_surface = anariNewSurface(m_device);
    if (m_surface) {
        anariSetParameter(m_device, m_surface, "geometry", ANARI_GEOMETRY, &m_geometry);
        if (m_material) {
            anariSetParameter(m_device, m_surface, "material", ANARI_MATERIAL, &m_material);
        }
        anariCommitParameters(m_device, m_surface);
    }

    if (m_surface) {
        m_group = anariNewGroup(m_device);
        ANARIArray1D surfaceArray =
            anariNewArray1D(m_device, &m_surface, nullptr, nullptr, ANARI_SURFACE, 1);
        anariSetParameter(m_device, m_group, "surface", ANARI_ARRAY1D, &surfaceArray);
        anariRelease(m_device, surfaceArray);
        anariCommitParameters(m_device, m_group);

        m_instance = anariNewInstance(m_device, "transform");
        if (m_instance && m_group) {
            anariSetParameter(m_device, m_instance, "group", ANARI_GROUP, &m_group);
            anariCommitParameters(m_device, m_instance);

            ANARIArray1D instanceArray =
                anariNewArray1D(m_device, &m_instance, nullptr, nullptr, ANARI_INSTANCE, 1);
            anariSetParameter(m_device, m_world, "instance", ANARI_ARRAY1D, &instanceArray);
            anariRelease(m_device, instanceArray);
            anariCommitParameters(m_device, m_world);
        }
    }

    emitStatus(LogLevel::Info,
               QStringLiteral("Loaded OBJ '%1' (%2 verts, %3 tris).")
                   .arg(path)
                   .arg(m_positions.size())
                   .arg(m_indices.size()));
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
    if (!m_device || !m_frame) {
        return;
    }

    // Two-state pump: kick a render on one tick, poll for completion on
    // subsequent ticks via ANARI_NO_WAIT. This keeps the GUI thread free
    // while the backend's own worker threads do the actual rendering.
    if (!m_frameInFlight) {
        anariRenderFrame(m_device, m_frame);
        m_frameInFlight = true;
        return;
    }

    if (anariFrameReady(m_device, m_frame, ANARI_NO_WAIT) != 1) {
        return;
    }

    uint32_t w = 0;
    uint32_t h = 0;
    ANARIDataType pixelType = ANARI_UNKNOWN;
    const void* pixels =
        anariMapFrame(m_device, m_frame, "channel.color", &w, &h, &pixelType);
    m_frameInFlight = false;

    if (!pixels || pixelType != ANARI_UFIXED8_VEC4) {
        if (pixels) {
            anariUnmapFrame(m_device, m_frame, "channel.color");
        }
        return;
    }

    if (w != static_cast<uint32_t>(m_frameImage.width()) ||
        h != static_cast<uint32_t>(m_frameImage.height())) {
        m_frameImage = QImage(static_cast<int>(w), static_cast<int>(h),
                              QImage::Format_RGBA8888);
    }

    std::memcpy(m_frameImage.bits(), pixels, static_cast<size_t>(w) * h * 4u);
    anariUnmapFrame(m_device, m_frame, "channel.color");

    // QImage uses implicit sharing, so emitting by value is cheap; receiver
    // gets its own snapshot if it later modifies it.
    emit frameReady(m_frameImage);
}

} // namespace vitrine
