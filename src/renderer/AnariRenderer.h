#pragma once

#include <anari/anari.h>

#include <QImage>
#include <QObject>
#include <QPointF>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector3D>

#include <array>
#include <memory>
#include <vector>

#include "AnariUtils.h"
#include "Camera.h"

class QTimer;

namespace vitrine
{

struct AnariScene
{
    ANARIFrame frame{nullptr};

    ANARIWorld world{nullptr};
    ANARIRenderer renderer{nullptr};
    ANARICamera camera{nullptr};

    // std::vector<ANARIInstance> instances;
    // std::vector<ANARIGroup> groups;

    std::vector<ANARISurface> surfaces;
    std::vector<ANARIVolume> volumes;
    std::vector<ANARILight> lights;

    /// @brief Release the geometry-level scene content (surfaces, groups,
    ///        instances, volumes) while leaving the frame / world / renderer
    ///        / camera / lights intact. Used when reloading a scene.
    void releaseContent(ANARIDevice device)
    {
        if (!device) {
            return;
        }
        // releaseHandles(device, instances);
        // releaseHandles(device, groups);
        releaseHandles(device, surfaces);
        releaseHandles(device, volumes);
    }

    /// @brief Release every ANARI handle owned by the scene. Used on full
    ///        backend teardown.
    void releaseSceneObjects(ANARIDevice device)
    {
        if (!device) {
            return;
        }
        releaseContent(device);
        releaseHandles(device, lights);
        releaseHandle(device, frame);
        releaseHandle(device, world);
        releaseHandle(device, renderer);
        releaseHandle(device, camera);
    }

private:
    template <typename T>
    static void releaseHandle(ANARIDevice device, T& handle)
    {
        if (handle) {
            anariRelease(device, handle);
            handle = nullptr;
        }
    }

    template <typename T>
    static void releaseHandles(ANARIDevice device, std::vector<T>& handles)
    {
        for (auto& handle : handles) {
            if (handle) {
                anariRelease(device, handle);
            }
        }
        handles.clear();
    }
};

/// @brief Owns the ANARI library / device / frame and emits the resulting
///        framebuffer as a QImage. Runs on the GUI thread (see the
///        Threading section in CLAUDE.md for why); the render loop is a
///        two-state pump driven by a QTimer with anariFrameReady(NO_WAIT).
class AnariRenderer : public QObject
{
    Q_OBJECT

public:
    explicit AnariRenderer(QObject* parent = nullptr);
    ~AnariRenderer() override;

    // Non-copyable, non-movable: we hold raw ANARI handles.
    AnariRenderer(const AnariRenderer&) = delete;
    AnariRenderer& operator=(const AnariRenderer&) = delete;

    /// @brief Returns the currently loaded library handle (null until loadBackend).
    /// Safe to read only from the renderer's owning thread.
    ANARILibrary library() const { return m_library; }
    ANARIDevice device() const { return m_device; }

public slots:
    /// @brief Load an ANARI backend library and create its default device.
    ///        Tears down any previously-loaded backend first. Emits
    ///        backendLoaded(true) on success, backendLoaded(false) on failure.
    void loadBackend(const QString& libraryName, const QString& deviceSubtype);

    /// @brief Switch the renderer subtype (e.g. "default", "raycast",
    ///        "pathtracer"). No-op if the device is not loaded.
    void setRendererSubtype(const QString& subtype);

    /// @brief Set a renderer parameter via the ANARI introspection types.
    ///        Variant is unpacked to the matching native type based on
    ///        `type`. Calls anariCommitParameters on the renderer.
    void setRendererParameter(const QString& name, int type, const QVariant& value);

    /// @brief Set the framebuffer size. Re-allocates the QImage buffer.
    void resize(const QSize& size);

    /// @brief Load a scene file from disk. DataLoaderFactory picks a loader
    ///        based on the file suffix; the loader populates the scene's
    ///        surfaces/groups/instances and attaches them to the world. Prior
    ///        scene content is released first. Unsupported suffixes are logged
    ///        and ignored.
    void loadSceneFromFile(const QString& path);

    /// @brief Orbit the camera around its center. Delta is the mouse drag in
    ///        pixels; mapped to yaw/pitch internally.
    void orbitCamera(const QPointF& deltaPixels);

    /// @brief Pan the camera (translate eye + center in the view plane).
    ///        Delta is the mouse drag in pixels, scaled by view distance.
    void panCamera(const QPointF& deltaPixels);

    /// @brief Dolly the camera toward/away from its center. `steps` is in
    ///        wheel notches (one notch == 120 raw units / 1.0 here).
    void dollyCamera(float steps);

    /// @brief Begin or resume the render loop.
    void start();

    /// @brief Pause the render loop; releases nothing.
    void stop();

signals:
    void backendLoaded(bool ok, const QString& libraryName, const QString& deviceSubtype);
    void frameReady(const QImage& image);
    void statusMessage(int level, const QString& message);

    /// @brief Emitted whenever the camera basis changes (load / orbit / pan /
    ///        dolly). Consumed by the axis-gizmo overlay. `forward` points
    ///        from the eye toward the look-at center.
    void cameraChanged(const QVector3D& right, const QVector3D& up, const QVector3D& forward);

private slots:
    void renderTick();

private:
    void destroyBackend();
    void rebuildFrame();
    void emitStatus(LogLevel level, const QString& message);

    /// @brief Re-push the camera state to the ANARI camera and emit
    ///        cameraChanged. No-op if the device/camera aren't ready.
    void commitCamera();

    std::unique_ptr<AnariStatusSink> m_statusSink;
    std::unique_ptr<Camera> m_camera;

    ANARILibrary m_library{nullptr};
    ANARIDevice m_device{nullptr};
    AnariScene m_anariScene;

    QString m_libraryName;
    QString m_deviceSubtype;
    QString m_rendererSubtype{QStringLiteral("default")};

    QSize m_size{1, 1};
    QImage m_frameImage;
    bool m_frameInFlight{false};
    QTimer* m_renderTimer{nullptr};
};

} // namespace vitrine
