#include "SceneAxisOverlay.h"

#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPaintEvent>
#include <QPointF>
#include <QVector3D>

#include <algorithm>
#include <array>

namespace vitrine
{

namespace
{
constexpr int kMargin = 8;       // px between the gizmo and the widget edge
constexpr qreal kSphereRadius = 7.0;  // px radius of the tip spheres

struct ProjectedAxis
{
    QPointF screen;  // tip offset from the gizmo center, in pixels
    float depth;     // along the view forward axis; larger == farther away
    QColor color;
    QChar label;
};
} // namespace

SceneAxisOverlay::SceneAxisOverlay(QWidget* parent)
    : QWidget(parent)
{
    // Float over the rendered frame without intercepting camera drags or
    // painting a background.
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFixedSize(100, 100);
}

void SceneAxisOverlay::setBasis(const QVector3D& right, const QVector3D& up, const QVector3D& forward)
{
    // Use the camera basis verbatim so the gizmo rotates identically to the
    // scene content under it (both are the world as the camera sees it).
    m_right = right;
    m_up = up;
    m_forward = forward;
    update();
}

void SceneAxisOverlay::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPointF center(width() / 2.0, height() / 2.0);
    // Leave room for the tip spheres so they never clip the widget edge.
    const float length =
        std::min(width(), height()) / 2.0f - kMargin - static_cast<float>(kSphereRadius);

    // Project each world axis into screen space: x along camera right, y along
    // camera up (negated because widget y grows downward). depth along forward
    // is used only to order the draw so nearer axes sit on top.
    const std::array<QVector3D, 3> worldAxes{
        QVector3D(1.0f, 0.0f, 0.0f),
        QVector3D(0.0f, 1.0f, 0.0f),
        QVector3D(0.0f, 0.0f, 1.0f)};
    const std::array<QColor, 3> colors{
        QColor(229, 76, 76),   // X - red
        QColor(102, 204, 102), // Y - green
        QColor(82, 148, 226)}; // Z - blue
    const std::array<QChar, 3> labels{QChar('X'), QChar('Y'), QChar('Z')};

    std::array<ProjectedAxis, 3> axes;
    for (int i = 0; i < 3; ++i) {
        const QVector3D& a = worldAxes[i];
        axes[i].screen = QPointF(QVector3D::dotProduct(a, m_right) * length,
                                 -QVector3D::dotProduct(a, m_up) * length);
        axes[i].depth = QVector3D::dotProduct(a, m_forward);
        axes[i].color = colors[i];
        axes[i].label = labels[i];
    }

    // Painter's algorithm: farther (larger depth) first.
    std::sort(axes.begin(), axes.end(),
              [](const ProjectedAxis& l, const ProjectedAxis& r) { return l.depth > r.depth; });

    QFont labelFont = painter.font();
    labelFont.setBold(true);
    labelFont.setPixelSize(static_cast<int>(kSphereRadius * 1.4));
    painter.setFont(labelFont);

    for (const ProjectedAxis& axis : axes) {
        const QPointF tip = center + axis.screen;

        // Axis line.
        QPen pen(axis.color);
        pen.setWidthF(2.0);
        painter.setPen(pen);
        painter.drawLine(center, tip);

        // Sphere at the tip, filled with the axis color.
        painter.setPen(Qt::NoPen);
        painter.setBrush(axis.color);
        painter.drawEllipse(tip, kSphereRadius, kSphereRadius);

        // Axis label, drawn in black on top of the sphere.
        painter.setPen(QColor(0, 0, 0));
        painter.drawText(QRectF(tip.x() - kSphereRadius, tip.y() - kSphereRadius,
                                2.0 * kSphereRadius, 2.0 * kSphereRadius),
                         Qt::AlignCenter, QString(axis.label));
    }

    // Origin dot on top of the lines.
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(220, 220, 220));
    painter.drawEllipse(center, 2.0, 2.0);
}

} // namespace vitrine
