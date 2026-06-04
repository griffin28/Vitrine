#pragma once

#include <QVector3D>
#include <QWidget>

class QPaintEvent;

namespace vitrine
{

/// @brief A small, transparent gizmo that draws the world X/Y/Z axes oriented
///        by the current camera view. It is meant to be parented to (and
///        manually positioned over) the AnariFrameWidget; it is transparent
///        to mouse events so camera drags pass straight through.
///
/// The widget knows nothing about ANARI or glm — it consumes only the camera's
/// orthonormal basis (right / up / forward) via setBasis().
class SceneAxisOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit SceneAxisOverlay(QWidget* parent = nullptr);
    ~SceneAxisOverlay() override = default;

public slots:
    /// @brief Update the camera basis used to orient the gizmo. `forward`
    ///        points from the eye toward the look-at center.
    void setBasis(const QVector3D& right, const QVector3D& up, const QVector3D& forward);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector3D m_right{1.0f, 0.0f, 0.0f};
    QVector3D m_up{0.0f, 1.0f, 0.0f};
    QVector3D m_forward{0.0f, 0.0f, -1.0f};
};

} // namespace vitrine
