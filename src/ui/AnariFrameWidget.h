#pragma once

#include <QImage>
#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QWidget>

class QMouseEvent;
class QWheelEvent;

namespace vitrine
{

/// @brief Displays the latest frame produced by AnariRenderer. The renderer
///        runs on a worker thread and pushes QImages here via the
///        updateFrame slot (queued connection); paintEvent simply blits the
///        cached image.
class AnariFrameWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AnariFrameWidget(QWidget* parent = nullptr);
    ~AnariFrameWidget() override = default;

    QSize sizeHint() const override { return QSize(800, 600); }

public slots:
    void updateFrame(const QImage& image);

signals:
    void resized(const QSize& size);

    // Camera-control gestures, in mouse-delta pixels / wheel notches. The
    // mouse-button -> gesture mapping lives here; the renderer interprets the
    // deltas against the active Camera.
    void orbitRequested(const QPointF& deltaPixels);
    void panRequested(const QPointF& deltaPixels);
    void dollyRequested(float steps);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    QImage m_image;

    // Drag tracking. m_dragButton is Qt::NoButton when no gesture is active.
    Qt::MouseButton m_dragButton{Qt::NoButton};
    bool m_panning{false};   // true when the active drag maps to pan
    QPoint m_lastPos;
};

} // namespace vitrine
