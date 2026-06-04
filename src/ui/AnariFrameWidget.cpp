#include "AnariFrameWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QWheelEvent>

namespace vitrine
{

AnariFrameWidget::AnariFrameWidget(QWidget* parent)
    : QWidget(parent)
{
    setAutoFillBackground(true);
    setMinimumSize(64, 64);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Receive wheel/drag events; we don't need move events without a button.
    setFocusPolicy(Qt::StrongFocus);
}

void AnariFrameWidget::updateFrame(const QImage& image)
{
    if (image.isNull()) {
        return;
    }
    m_image = image;
    update();
}

void AnariFrameWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    if (m_image.isNull()) {
        return;
    }
    if (m_image.size() == size()) {
        painter.drawImage(0, 0, m_image);
    } 
    else {
        // Renderer hasn't caught up to the new widget size yet — scale the
        // last frame to fill so we don't blink to black on every resize.
        painter.drawImage(rect(), m_image);
    }
}

void AnariFrameWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    emit resized(event->size());
}

void AnariFrameWidget::mousePressEvent(QMouseEvent* event)
{
    // Left drag orbits; middle drag (or Shift+Left) pans.
    if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) {
        m_dragButton = event->button();
        m_panning = event->button() == Qt::MiddleButton ||
                    (event->modifiers() & Qt::ShiftModifier);
        m_lastPos = event->pos();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void AnariFrameWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragButton == Qt::NoButton) {
        QWidget::mouseMoveEvent(event);
        return;
    }
    const QPoint delta = event->pos() - m_lastPos;
    m_lastPos = event->pos();
    if (m_panning) {
        emit panRequested(QPointF(delta));
    } else {
        emit orbitRequested(QPointF(delta));
    }
    event->accept();
}

void AnariFrameWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == m_dragButton) {
        m_dragButton = Qt::NoButton;
        m_panning = false;
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void AnariFrameWidget::wheelEvent(QWheelEvent* event)
{
    // angleDelta is in eighths of a degree; one notch == 120 units.
    const float steps = static_cast<float>(event->angleDelta().y()) / 120.0f;
    if (steps != 0.0f) {
        emit dollyRequested(steps);
        event->accept();
        return;
    }
    QWidget::wheelEvent(event);
}

} // namespace vitrine
