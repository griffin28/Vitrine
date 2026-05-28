#pragma once

#include <QImage>
#include <QSize>
#include <QWidget>

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

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QImage m_image;
};

} // namespace vitrine
