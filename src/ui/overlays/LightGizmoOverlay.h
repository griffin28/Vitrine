#pragma once

#include <QMatrix4x4>
#include <QVector3D>
#include <QWidget>

#include <vector>

#include "LightConfig.h"

class QPaintEvent;

namespace vitrine
{

/// @class LightGizmoOverlay
/// @brief Transparent overlay that draws the scene's lights over the frame.
///
/// Each enabled light is projected through the camera's view-projection matrix
/// and drawn in its own color: a sun-like marker for point lights, an arrow
/// along the direction of travel for directional lights, and a cone outline for
/// spot lights. Directional lights have no position, so their arrow is anchored
/// on the camera's look-at center.
///
/// The widget is parented to, and sized to cover, the AnariFrameWidget and is
/// transparent to mouse events so camera drags pass through.
class LightGizmoOverlay : public QWidget
{
    Q_OBJECT

public:
    /// @brief Constructor
    /// @param parent parent widget
    explicit LightGizmoOverlay(QWidget* parent = nullptr);

    /// @brief Destructor
    ~LightGizmoOverlay() override = default;

    /// @brief Pixel radius of the point/spot light marker
    static constexpr qreal kMarkerRadius = 6.0;

public slots:
    /// @brief Set the lights to draw
    /// @param lights light configurations; disabled entries are skipped
    void setLights(const std::vector<LightConfig>& lights);

    /// @brief Update the camera used to project the gizmos
    /// @param viewProjection world-to-clip matrix (OpenGL conventions)
    /// @param center camera look-at center in world space
    /// @param distance eye-to-center distance, used to scale gizmo lengths
    void setCamera(const QMatrix4x4& viewProjection, const QVector3D& center, float distance);

protected:
    /// @brief Paint the light gizmos
    /// @param event paint event
    void paintEvent(QPaintEvent* event) override;

private:
    /// @brief Project a world-space point to widget pixels
    /// @param world world-space point
    /// @param[out] screen widget-space position
    /// @return false if the point is behind the camera
    bool project(const QVector3D& world, QPointF& screen) const;

    std::vector<LightConfig> m_lights;
    QMatrix4x4 m_viewProjection;
    QVector3D m_center;
    float m_distance{1.0f};
};

} // namespace vitrine
