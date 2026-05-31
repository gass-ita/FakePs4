#include "Canvas.h"
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include "Shape.h"

Canvas::Canvas(QWidget *parent)
    : QWidget(parent), layerManager(1920, 1080)
{
    setFixedSize(layerManager.getWidth(), layerManager.getHeight());
    layerManager.addObserver(this);

    currentTool = std::make_shared<BrushTool>();

    panOffset = QPointF(100, 100); // Start with a pan offset to center the canvas
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

void Canvas::addNewLayer(const QString &name)
{
    layerManager.addLayer(name.toStdString());
}

void Canvas::setActiveLayer(int index)
{
    layerManager.setActiveLayer(index);
}

void Canvas::setLayerVisibility(int index, bool visible)
{
    layerManager.setLayerVisibility(index, visible);
}

void Canvas::setLayerName(int index, const QString &name)
{
    layerManager.setLayerName(index, name.toStdString());
}

// ==========================================
// VIEWPORT MATH
// ==========================================

QPoint Canvas::screenToImage(const QPoint &screenPos) const
{
    // Reverse the zoom and pan math to find the exact pixel the user clicked
    return QPoint(
        (screenPos.x() - panOffset.x()) / zoom,
        (screenPos.y() - panOffset.y()) / zoom);
}

void Canvas::onRegionChanged(int x, int y, int w, int h)
{
    // The core tells us an image rectangle changed.
    // We must convert that to a screen rectangle for Qt to update!
    int sx = (x * zoom) + panOffset.x();
    int sy = (y * zoom) + panOffset.y();
    int sw = (w * zoom);
    int sh = (h * zoom);

    // Add a 2-pixel margin to account for floating point rounding errors
    this->update(QRect(sx - 2, sy - 2, sw + 4, sh + 4));
}
