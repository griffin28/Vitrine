#pragma once

#include <anari/anari.h>

#include <QImage>
#include <QObject>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <array>
#include <memory>
#include <vector>

#include "AnariUtils.h"

class QTimer;

namespace vitrine
{

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

    /// @brief Load an OBJ from disk, dedup vertices, and push the mesh
    ///        through ANARI as a triangle geometry under a single surface.
    ///        On backends that don't implement triangle geometry yet the
    ///        calls return invalid handles and we log+continue.
    void setSceneFromObj(const QString& path);

    /// @brief Begin or resume the render loop.
    void start();

    /// @brief Pause the render loop; releases nothing.
    void stop();

signals:
    void backendLoaded(bool ok, const QString& libraryName, const QString& deviceSubtype);
    void frameReady(const QImage& image);
    void statusMessage(int level, const QString& message);

private slots:
    void renderTick();

private:
    void destroyBackend();
    void rebuildFrame();
    void emitStatus(LogLevel level, const QString& message);

    std::unique_ptr<AnariStatusSink> m_statusSink;

    ANARILibrary m_library{nullptr};
    ANARIDevice m_device{nullptr};
    ANARIRenderer m_renderer{nullptr};
    ANARICamera m_camera{nullptr};
    ANARIWorld m_world{nullptr};
    ANARIFrame m_frame{nullptr};
    ANARIGeometry m_geometry{nullptr};
    ANARISurface m_surface{nullptr};
    ANARIMaterial m_material{nullptr};
    ANARIGroup m_group{nullptr};
    ANARIInstance m_instance{nullptr};

    QString m_libraryName;
    QString m_deviceSubtype;
    QString m_rendererSubtype{QStringLiteral("default")};

    QSize m_size{1, 1};
    QImage m_frameImage;
    bool m_frameInFlight{false};
    QTimer* m_renderTimer{nullptr};

    // Cached mesh data the ANARI arrays reference.
    std::vector<std::array<float, 3>> m_positions;
    std::vector<std::array<uint32_t, 3>> m_indices;
};

} // namespace vitrine
