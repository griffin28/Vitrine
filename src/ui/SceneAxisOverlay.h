#pragma once

#include <QVector3D>
#include <QWidget>

class QPaintEvent;

namespace vitrine
{

/// @brief Projected screen-space representation of one axis
struct ProjectedAxis
{
    QPointF screen;  // Tip offset from the gizmo center, in pixels.
    float depth;     // Along the view forward axis; larger == farther away.
    QColor color;
    QChar label;
};

/// @class SceneAxisOverlay
/// @brief Scene axis overlay widget.
///
/// SceneAxisOverlay draws a small transparent gizmo showing the world X/Y/Z
/// axes oriented by the current camera view.
///
/// The widget is meant to be parented to, and manually positioned over, the
/// AnariFrameWidget. It consumes only the camera orthonormal basis through
/// setBasis() and remains transparent to mouse events so camera drags pass
/// through to the frame widget.
class SceneAxisOverlay : public QWidget
{
    Q_OBJECT

public:
    /// @brief Constructor
    /// @param parent parent widget
    explicit SceneAxisOverlay(QWidget* parent = nullptr);

    /// @brief Destructor
    ~SceneAxisOverlay() override = default;

    /// @brief Pixel margin between the gizmo and the widget edge
    static constexpr int kMargin = 8;

    /// @brief Pixel radius of the axis tip spheres
    static constexpr qreal kSphereRadius = 7.0;

public slots:
    /// @brief Update the camera basis used to orient the gizmo
    /// @param right camera right vector
    /// @param up camera up vector
    /// @param forward camera forward vector from the eye toward the look-at center
    void setBasis(const QVector3D& right, const QVector3D& up, const QVector3D& forward);

protected:
    /// @brief Paint the axis overlay
    /// @param event paint event
    void paintEvent(QPaintEvent* event) override;

private:
    QVector3D m_right{1.0f, 0.0f, 0.0f};
    QVector3D m_up{0.0f, 1.0f, 0.0f};
    QVector3D m_forward{0.0f, 0.0f, -1.0f};
};

} // namespace vitrine
