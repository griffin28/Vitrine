#include "AnariFrameWidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>

namespace vitrine
{

AnariFrameWidget::AnariFrameWidget(QWidget* parent)
    : QWidget(parent)
{
    setAutoFillBackground(true);
    setMinimumSize(64, 64);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
    } else {
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

} // namespace vitrine
