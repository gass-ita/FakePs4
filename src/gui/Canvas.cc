#include "Canvas.h"
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include "Shape.h"
#include <iostream>

Canvas::Canvas(int canvasWidth, int canvasHeight, QWidget *parent)
    : QWidget(parent), layerManager(canvasWidth, canvasHeight) // Initialize LayerManager with canvas size
{
    // setFixedSize(layerManager.getWidth(), layerManager.getHeight());
    layerManager.addObserver(this);
    setMouseTracking(true); // Enable mouse move events even when no buttons are pressed

    // set the layer manager's tool to a default brush so we have a valid tool to call on hover before the user selects one
    layerManager.setActiveTool(std::make_unique<PressureBrushTool>());
    layerManager.setToolSize(5);
    layerManager.setToolColor(255, 0, 0, 255);

    panOffset = QPointF(100, 100); // Start with a pan offset to center the canvas
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

// ==========================================
// INTERACTION (Panning and Drawing)
// ==========================================

void Canvas::tabletEvent(QTabletEvent *event)
{
    QPoint imgPos = screenToImage(event->position().toPoint());

    float pressure = event->pressure();
    float tiltX = event->xTilt();
    float tiltY = event->yTilt();

    if (!layerManager.getActiveTool())
        return;

    if (event->type() == QEvent::TabletPress)
    {
        // Tell the engine the pen touched the screen
        layerManager.onPress(imgPos.x(), imgPos.y(), pressure, tiltX, tiltY);

        setCursor(Qt::CrossCursor); // Visual feedback for drawing
    }
    else if (event->type() == QEvent::TabletMove)
    {
        // 1. Pass the movement data to the engine.
        // (If the pen is just hovering, the Tool's internal 'if (!isDrawing)' check will safely ignore this!)
        layerManager.onMove(imgPos.x(), imgPos.y(), pressure, tiltX, tiltY);

        // 2. Always tell the engine where the pen is hovering so it can draw the cursor preview ring
        layerManager.onHover(imgPos.x(), imgPos.y());
    }
    else if (event->type() == QEvent::TabletRelease)
    {
        // Tell the engine the pen lifted
        unsetCursor();
        layerManager.onRelease(imgPos.x(), imgPos.y(), pressure, tiltX, tiltY);
    }

    // Accept the event so Qt doesn't fire a duplicate mouse event
    event->accept();
}

void Canvas::mousePressEvent(QMouseEvent *event)
{
    // Middle Mouse Button -> Start Panning
    if (event->button() == Qt::MiddleButton)
    {
        isPanning = true;
        lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor); // Visual feedback
    }
    // Left Mouse Button -> Pass to Tool
    else if (event->button() == Qt::LeftButton && layerManager.getActiveTool())
    {
        setCursor(Qt::CrossCursor); // Visual feedback for drawing
        QPoint imgPos = screenToImage(event->pos());
        layerManager.onPress(imgPos.x(), imgPos.y(), 1.0f, 0.0f, 0.0f);
    }
}

void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    QPoint imgPos = screenToImage(event->pos());
    Tool *activeTool = layerManager.getActiveTool();

    // 1. Panning (Canvas handles this because pan isn't part of the core image)
    if (isPanning)
    {
        QPoint delta = event->pos() - lastPanPos;
        panOffset += delta;
        lastPanPos = event->pos();
        update(); // Canvas must update itself when panning
        return;
    }

    // 2. Pass data to the Core (Let the core trigger onRegionChanged!)
    if ((event->buttons() & Qt::LeftButton) && activeTool)
    {
        layerManager.onMove(imgPos.x(), imgPos.y(), 1.0f, 0.0f, 0.0f);
    }

    if (activeTool)
    {
        layerManager.onHover(imgPos.x(), imgPos.y());
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event)
{
    unsetCursor();
    if (event->button() == Qt::MiddleButton)
    {
        isPanning = false;
    }
    else if (event->button() == Qt::LeftButton && layerManager.getActiveTool())
    {
        QPoint imgPos = screenToImage(event->pos());
        layerManager.onRelease(imgPos.x(), imgPos.y(), 1.0f, 0.0f, 0.0f);
    }
}

void Canvas::wheelEvent(QWheelEvent *event)
{
    float oldZoom = zoom;

    // Standard UI logic: scroll up zooms in, scroll down zooms out
    if (event->angleDelta().y() > 0)
    {
        zoom *= 1.15f;
    }
    else
    {
        zoom /= 1.15f;
    }

    // Clamp the zoom so the user doesn't crash the math (10% to 3000%)
    zoom = std::clamp(zoom, 0.1f, 30.0f);

    // Zoom-To-Cursor Math: Adjust pan offset so the pixel under the mouse stays exactly under the mouse
    QPointF mousePos = event->position();
    panOffset.setX(mousePos.x() - (mousePos.x() - panOffset.x()) * (zoom / oldZoom));
    panOffset.setY(mousePos.y() - (mousePos.y() - panOffset.y()) * (zoom / oldZoom));
    syncScrollbars();
    update(); // Redraw the screen
    emit zoomChanged(static_cast<int>(zoom * 100));
}

// ==========================================
// RENDERING
// ==========================================

void Canvas::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    // 1. Draw a dark grey background for the infinite workspace
    painter.fillRect(rect(), QColor(40, 40, 40));

    // 2. Figure out which part of the IMAGE is exposed by this screen update
    QRect screenDirty = event->rect();
    int ix = (screenDirty.x() - panOffset.x()) / zoom;
    int iy = (screenDirty.y() - panOffset.y()) / zoom;
    int iw = (screenDirty.width() / zoom) + 2;
    int ih = (screenDirty.height() / zoom) + 2;

    // 3. Clamp the request so we don't ask the LayerManager for out-of-bounds memory
    int realX = std::max(0, ix);
    int realY = std::max(0, iy);
    int realW = std::min(layerManager.getWidth() - realX, iw);
    int realH = std::min(layerManager.getHeight() - realY, ih);

    // If the dirty screen rect doesn't touch the image at all, stop here
    if (realW <= 0 || realH <= 0)
        return;

    // 4. THE CORRECTION: Pass our persistent frameBuffer by reference!
    // This entirely eliminates the heap allocation stutter.
    layerManager.renderRegion(realX, realY, realW, realH, frameBuffer);

    // 5. Wrap the memory in a Qt Image
    // Because frameBuffer belongs to the class, this memory is guaranteed to be stable.
    // Make sure you use the 'bytesPerLine' argument (realW * 4) to prevent row-alignment skewing!
    QImage image(frameBuffer.data(), realW, realH, realW * 4, QImage::Format_RGBA8888);

    // 6. Use Qt's hardware scaling to put it on the screen perfectly
    painter.translate(panOffset.x() + (realX * zoom), panOffset.y() + (realY * zoom));
    painter.scale(zoom, zoom);

    // Turn off anti-aliasing so the pixels stay sharp when zoomed in (like Photoshop)
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    painter.drawImage(0, 0, image);
}

void Canvas::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
}

// ==========================================
// SCROLLBAR SYNCHRONIZATION
// ==========================================

void Canvas::syncScrollbars()
{
    // 1. Calculate how big the image is on the screen right now
    int scaledWidth = layerManager.getWidth() * zoom;
    int scaledHeight = layerManager.getHeight() * zoom;

    // 2. The maximum we can scroll is the difference between the image size and the window size
    // We add a little padding (width() / 2) so the user can scroll past the edge slightly
    int maxX = std::max(0, scaledWidth - (width() / 2));
    int maxY = std::max(0, scaledHeight - (height() / 2));

    // 3. Emit the limits so the MainWindow can set up the UI bars
    emit scrollRangeChanged(maxX, maxY, width(), height());

    // 4. Convert panOffset to scrollbar values
    // panOffset is negative when the image moves left/up. Scrollbars use positive values.
    emit scrollValueChanged(-panOffset.x(), -panOffset.y());
}

void Canvas::setScrollX(int value)
{
    panOffset.setX(-value);
    update(); // Redraw the screen
}

void Canvas::setScrollY(int value)
{
    panOffset.setY(-value);
    update();
}

void Canvas::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    syncScrollbars(); // Recalculate if the user maximizes the window!
}

// ==========================================
// UI ZOOM CONTROLS
// ==========================================
void Canvas::setZoom(float newZoom)
{
    // 1. Clamp it to the exact same limits as the scroll wheel
    newZoom = std::clamp(newZoom, 0.1f, 30.0f);

    // 2. If the zoom didn't actually change, do nothing
    if (newZoom == zoom)
        return;

    float oldZoom = zoom;
    zoom = newZoom;

    // 3. Zoom-To-Center Math: Keep the dead-center of the screen anchored!
    QPointF centerPos(width() / 2.0, height() / 2.0);
    panOffset.setX(centerPos.x() - (centerPos.x() - panOffset.x()) * (zoom / oldZoom));
    panOffset.setY(centerPos.y() - (centerPos.y() - panOffset.y()) * (zoom / oldZoom));

    // 4. Update everything
    syncScrollbars();
    update();

    // 5. Tell the UI Text Box to update!
    emit zoomChanged(static_cast<int>(zoom * 100));
}

void Canvas::zoomIn()
{
    setZoom(zoom * 1.15f);
}

void Canvas::zoomOut()
{
    setZoom(zoom / 1.15f);
}
