#include "Canvas.h"
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include "Shape.h"

Canvas::Canvas(QWidget *parent)
    : QWidget(parent), layerManager(800, 600)
{
    setFixedSize(layerManager.getWidth(), layerManager.getHeight());
    layerManager.addObserver(this);

    currentTool = std::make_shared<BrushTool>();
}

void Canvas::onRegionChanged(int x, int y, int w, int h)
{
    this->update(QRect(x, y, w, h));
}

void Canvas::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && currentTool)
    {
        currentTool->onPress(event->pos().x(), event->pos().y(), layerManager);
    }
}

void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) && currentTool)
    {
        currentTool->onMove(event->pos().x(), event->pos().y(), layerManager);
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && currentTool)
    {
        currentTool->onRelease(event->pos().x(), event->pos().y(), layerManager);
    }
}

void Canvas::paintEvent(QPaintEvent *event)
{
    QRect dirtyRect = event->rect();
    std::vector<uint8_t> buffer = layerManager.renderRegion(
        dirtyRect.x(), dirtyRect.y(), dirtyRect.width(), dirtyRect.height());

    if (buffer.empty())
        return;

    QImage image(buffer.data(), dirtyRect.width(), dirtyRect.height(), QImage::Format_RGBA8888);
    QPainter painter(this);
    painter.drawImage(dirtyRect.topLeft(), image);
}