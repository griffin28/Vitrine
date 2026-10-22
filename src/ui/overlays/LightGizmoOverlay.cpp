#include "LightGizmoOverlay.h"

#include <QColor>
#include <QPainter>
#include <QPaintEvent>
#include <QPointF>
#include <QVector4D>

#include <algorithm>
#include <cmath>

namespace vitrine
{

namespace
{
QVector3D toQt(const glm::vec3& v)
{
    return QVector3D(v.x, v.y, v.z);
}

QColor toQColor(const glm::vec3& c)
{
    const auto ch = [](float v) { return std::clamp(static_cast<int>(v * 255.0f + 0.5f), 0, 255); };
    return QColor(ch(c.x), ch(c.y), ch(c.z));
}

// Pick a vector perpendicular to the unit vector `d`.
QVector3D perpendicular(const QVector3D& d)
{
    const QVector3D axis = std::abs(d.y()) < 0.9f ? QVector3D(0, 1, 0) : QVector3D(1, 0, 0);
    return QVector3D::crossProduct(d, axis).normalized();
}
} // namespace

LightGizmoOverlay::LightGizmoOverlay(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
}

void LightGizmoOverlay::setLights(const std::vector<LightConfig>& lights)
{
    m_lights = lights;
    update();
}

void LightGizmoOverlay::setCamera(const QMatrix4x4& viewProjection,
                                  const QVector3D& center,
                                  float distance)
{
    m_viewProjection = viewProjection;
    m_center = center;
    m_distance = distance;
    update();
}

bool LightGizmoOverlay::project(const QVector3D& world, QPointF& screen) const
{
    const QVector4D clip = m_viewProjection * QVector4D(world, 1.0f);
    if (clip.w() <= 1.0e-6f) 
    {
        return false;
    }
    const float nx = clip.x() / clip.w();
    const float ny = clip.y() / clip.w();
    screen = QPointF((nx * 0.5f + 0.5f) * width(), (1.0f - (ny * 0.5f + 0.5f)) * height());
    return true;
}

void LightGizmoOverlay::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const float len = std::max(m_distance, 1.0e-3f) * 0.3f;

    for (const LightConfig& light : m_lights) 
    {
        if (!light.enabled) 
        {
            continue;
        }

        const QColor color = toQColor(light.color);
        // Dark halo under the colored stroke keeps gizmos legible on bright
        // and dark parts of the render alike.
        QPen halo(QColor(0, 0, 0, 160));
        halo.setWidthF(4.0);
        QPen pen(color);
        pen.setWidthF(2.0);
        const auto stroke = [&](const QPointF& a, const QPointF& b) {
            painter.setPen(halo);
            painter.drawLine(a, b);
            painter.setPen(pen);
            painter.drawLine(a, b);
        };

        QVector3D dir = toQt(light.direction);
        dir = dir.lengthSquared() > 1.0e-12f ? dir.normalized() : QVector3D(0, 0, -1);

        if (light.type == LightType::Directional) 
        {
            // Arrow of travel through the look-at center.
            const QVector3D tailW = m_center - dir * len;
            QPointF tail, head;
            if (!project(tailW, tail) || !project(m_center, head)) 
            {
                continue;
            }
            stroke(tail, head);

            // Arrowhead.
            const QPointF v = head - tail;
            const double vl = std::hypot(v.x(), v.y());
            if (vl > 1.0) 
            {
                const QPointF u = v / vl;
                const QPointF n(-u.y(), u.x());
                stroke(head, head - u * 10.0 + n * 5.0);
                stroke(head, head - u * 10.0 - n * 5.0);
            }
            // Sun disc at the tail.
            painter.setPen(halo);
            painter.setBrush(color);
            painter.drawEllipse(tail, kMarkerRadius, kMarkerRadius);
            continue;
        }

        QPointF p;
        const QVector3D posW = toQt(light.position);
        if (!project(posW, p)) 
        {
            continue;
        }

        if (light.type == LightType::Spot) 
        {
            // Cone outline: axis plus four rim points at the cone's opening.
            const float coneLen = len * 0.8f;
            const float halfAngle = glm::radians(std::clamp(light.openingAngleDegrees, 1.0f, 170.0f)) * 0.5f;
            const float rimRadius = coneLen * std::tan(std::min(halfAngle, glm::radians(80.0f)));
            const QVector3D tipW = posW + dir * coneLen;
            const QVector3D u = perpendicular(dir);
            const QVector3D w = QVector3D::crossProduct(dir, u);

            QPointF tip;
            if (project(tipW, tip)) 
            {
                stroke(p, tip);
            }
            QPointF prev;
            bool havePrev = false;
            constexpr int kRimSegments = 24;
            for (int i = 0; i <= kRimSegments; ++i) 
            {
                const float t = 2.0f * static_cast<float>(M_PI) * i / kRimSegments;
                const QVector3D rimW = tipW + (u * std::cos(t) + w * std::sin(t)) * rimRadius;
                QPointF rim;
                const bool ok = project(rimW, rim);
                if (ok && havePrev) 
                {
                    stroke(prev, rim);
                }
                if (ok && i % (kRimSegments / 4) == 0 && i < kRimSegments) 
                {
                    stroke(p, rim);
                }
                prev = rim;
                havePrev = ok;
            }
        }

        // Marker at the light position.
        painter.setPen(halo);
        painter.setBrush(color);
        painter.drawEllipse(p, kMarkerRadius, kMarkerRadius);
        if (light.type == LightType::Point) 
        {
            // Rays around the disc.
            for (int i = 0; i < 8; ++i) 
            {
                const double a = i * M_PI / 4.0;
                const QPointF d(std::cos(a), std::sin(a));
                stroke(p + d * (kMarkerRadius + 3.0), p + d * (kMarkerRadius + 8.0));
            }
        }
    }
}

} // namespace vitrine
