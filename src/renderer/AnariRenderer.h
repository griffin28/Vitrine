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

/// @struct AnariScene
/// @brief Owned ANARI handles for the currently loaded scene.
///
/// AnariScene groups the frame, world, renderer, camera, and scene-object
/// handles managed by AnariRenderer so they can be released consistently
/// during scene reloads and backend teardown.
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

    /// @brief Release scene content while keeping persistent renderer handles
    /// @param device ANARI device that owns the scene handles
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

    /// @brief Release every ANARI handle owned by the scene
    /// @param device ANARI device that owns the scene handles
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
    /// @brief Release a single ANARI handle and clear it
    /// @tparam T ANARI handle type
    /// @param device ANARI device that owns the handle
    /// @param handle handle to release and reset
    template <typename T>
    static void releaseHandle(ANARIDevice device, T& handle)
    {
        if (handle) {
            anariRelease(device, handle);
            handle = nullptr;
        }
    }

    /// @brief Release a vector of ANARI handles and clear it
    /// @tparam T ANARI handle type
    /// @param device ANARI device that owns the handles
    /// @param handles handles to release and clear
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

/// @class AnariRenderer
/// @brief ANARI rendering controller.
///
/// AnariRenderer owns the loaded ANARI library, device, scene handles, camera,
/// and render loop. It renders frames through ANARI and emits the resulting
/// framebuffer as a QImage for the UI.
///
/// The renderer lives on the GUI thread and uses a QTimer-driven two-state
/// render pump with anariFrameReady(ANARI_NO_WAIT) so the backend can render
/// asynchronously without blocking UI updates.
class AnariRenderer : public QObject
{
    Q_OBJECT

public:
    /// @brief Constructor
    /// @param parent parent QObject
    explicit AnariRenderer(QObject* parent = nullptr);

    /// @brief Destructor
    ~AnariRenderer() override;

    /// @brief Copy constructor
    AnariRenderer(const AnariRenderer&) = delete;

    /// @brief Copy assignment operator
    /// @return reference to this renderer
    AnariRenderer& operator=(const AnariRenderer&) = delete;

    /// @brief Get the currently loaded ANARI library handle
    /// @return loaded ANARI library handle, or nullptr before loadBackend()
    ANARILibrary library() const { return m_library; }

    /// @brief Get the currently loaded ANARI device handle
    /// @return loaded ANARI device handle, or nullptr before loadBackend()
    ANARIDevice device() const { return m_device; }

    /// @brief Get the current camera configuration
    /// @return camera configuration snapshot
    CameraConfig cameraConfig() const;

    /// @brief Destroy the current ANARI backend and release its handles
    void destroyBackend();

    /// @brief Convert a QVariant to an ANARI FLOAT32_VEC4 value
    /// @param v variant containing a QColor or four numeric values
    /// @param out destination array for the converted values
    /// @return true if conversion succeeded, false otherwise
    static QVariant unwrapVariantToFloat32Vec4(const QVariant& v, std::array<float, 4>& out);

    /// @brief Orbit sensitivity in radians per mouse pixel
    static constexpr float kOrbitRadiansPerPixel = 0.01f;

    /// @brief Pan sensitivity as a fraction of camera distance per mouse pixel
    static constexpr float kPanFractionPerPixel = 0.002f;  // scaled by view distance

    /// @brief Dolly sensitivity as a fraction of camera distance per wheel step
    static constexpr float kDollyFractionPerStep = 0.1f;   // scaled by view distance

    /// @brief Render loop interval in milliseconds
    static constexpr int kRenderIntervalMs = 16;

public slots:
    /// @brief Load an ANARI backend library and create its device
    /// @param libraryName ANARI library name to load
    /// @param deviceSubtype ANARI device subtype to create
    void loadBackend(const QString& libraryName, const QString& deviceSubtype);

    /// @brief Set the active ANARI renderer subtype
    /// @param subtype renderer subtype name, such as "default"
    void setRendererSubtype(const QString& subtype);

    /// @brief Set an ANARI renderer parameter
    /// @param name renderer parameter name
    /// @param type ANARI data type for the parameter
    /// @param value value to unpack and pass to ANARI
    void setRendererParameter(const QString& name, int type, const QVariant& value);

    /// @brief Set the framebuffer size
    /// @param size new framebuffer size
    void resize(const QSize& size);

    /// @brief Load a scene file from disk
    /// @param path scene file path
    void loadSceneFromFile(const QString& path);

    /// @brief Orbit the camera around its center
    /// @param deltaPixels mouse drag delta in pixels
    void orbitCamera(const QPointF& deltaPixels);

    /// @brief Pan the camera in the view plane
    /// @param deltaPixels mouse drag delta in pixels
    void panCamera(const QPointF& deltaPixels);

    /// @brief Dolly the camera toward or away from its center
    /// @param steps wheel-step delta
    void dollyCamera(float steps);

    /// @brief Apply a full camera configuration
    /// @param config camera configuration to apply
    void setCameraConfig(const CameraConfig& config);

    /// @brief Begin or resume the render loop
    void start();

    /// @brief Pause the render loop
    void stop();

signals:
    /// @brief Signal emitted after backend load succeeds or fails
    /// @param ok true if the backend loaded successfully, false otherwise
    /// @param libraryName ANARI library name used for the load attempt
    /// @param deviceSubtype ANARI device subtype used for the load attempt
    void backendLoaded(bool ok, const QString& libraryName, const QString& deviceSubtype);

    /// @brief Signal emitted when a rendered frame is ready for display
    /// @param image rendered framebuffer image
    void frameReady(const QImage& image);

    /// @brief Signal emitted when the renderer has a status message
    /// @param level log level as an integer LogLevel value
    /// @param message status message text
    void statusMessage(int level, const QString& message);

    /// @brief Signal emitted when the camera basis changes
    /// @param right camera right basis vector
    /// @param up camera up basis vector
    /// @param forward camera forward basis vector from eye toward center
    void cameraChanged(const QVector3D& right, const QVector3D& up, const QVector3D& forward);

private slots:
    /// @brief Advance the timer-driven render loop
    void renderTick();

private:
    /// @brief Rebuild the ANARI frame object and its parameters
    void rebuildFrame();

    /// @brief Emit a renderer status message
    /// @param level log level
    /// @param message status message text
    void emitStatus(LogLevel level, const QString& message);

    /// @brief Commit camera state to ANARI and emit cameraChanged()
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
