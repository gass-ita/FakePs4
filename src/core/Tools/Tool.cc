#include "Tool.h"
#include <iostream>

// ==========================================
// BRUSH TOOL
// ==========================================
void BrushTool::onPress(int x, int y, LayerManager &manager)
{
    manager.clearPreview();
    isDrawing = true;
    lastX = x;
    lastY = y;

    manager.beginBatch();
    Line(x, y, x, y, r, g, b, a, size).draw(manager);
    manager.endBatch();
}

void BrushTool::onHover(int x, int y, LayerManager &manager)
{
    if (isDrawing)
        return; // Don't show the hover cursor if we're actively drawing
    // 1. Wipe the cursor from the last mouse position
    manager.clearPreview();

    // 2. Explicitly draw a dark gray outline to the scratchpad
    // Notice the &LayerManager::setPreviewPixel argument!
    CircleOutline(x, y, size, 50, 50, 50, 255).draw(manager, &LayerManager::setPreviewPixel);

    // 3. Ask Qt to repaint that tiny bounding box
    manager.showPreview();
}
void BrushTool::onMove(int x, int y, LayerManager &manager)
{
    if (!isDrawing)
        return;

    manager.beginBatch();
    Line(lastX, lastY, x, y, r, g, b, a, size).draw(manager);
    manager.endBatch();

    lastX = x;
    lastY = y;
}

void BrushTool::onRelease(int x, int y, LayerManager &manager)
{
    isDrawing = false;
}

// ==========================================
// RECTANGLE TOOL (Click and Drag)
// ==========================================
void RectangleTool::onPress(int x, int y, LayerManager &manager)
{
    manager.clearPreview(); // Failsafe: clear any leftover hover cursors
    startX = x;
    startY = y;
    isDrawing = true;
}

void RectangleTool::onMove(int x, int y, LayerManager &manager)
{
    if (!isDrawing)
        return;

    // 1. Wipe the preview frame from the last mouse coordinate
    manager.clearPreview();

    int rectX = std::min(startX, x);
    int rectY = std::min(startY, y);
    int width = std::abs(x - startX);
    int height = std::abs(y - startY);

    if (width > 0 && height > 0)
    {
        // 2. Explicitly route the rectangle to the scratchpad memory!
        Rectangle(rectX, rectY, width, height, r, g, b, a, size).draw(manager, &LayerManager::setPreviewPixel);

        // 3. Ask Qt to repaint just that box
        manager.showPreview();
    }
}

void RectangleTool::onRelease(int x, int y, LayerManager &manager)
{
    if (!isDrawing)
        return;

    // CRITICAL: Wipe the temporary scratchpad shape completely!
    manager.clearPreview();

    int rectX = std::min(startX, x);
    int rectY = std::min(startY, y);
    int width = std::abs(x - startX);
    int height = std::abs(y - startY);

    if (width > 0 && height > 0)
    {
        manager.beginBatch();
        // Default routing: This burns into the REAL active layer
        Rectangle(rectX, rectY, width, height, r, g, b, a, size).draw(manager);
        manager.endBatch();
    }

    isDrawing = false;
}

// ==========================================
// ELLIPSE TOOL (Click and Drag)
// ==========================================
void EllipseTool::onPress(int x, int y, LayerManager &manager)
{
    manager.clearPreview();
    startX = x;
    startY = y;
    isDrawing = true;
}

void EllipseTool::onMove(int x, int y, LayerManager &manager)
{
    if (!isDrawing)
        return;

    manager.clearPreview();

    int rx = std::abs(x - startX) / 2;
    int ry = std::abs(y - startY) / 2;
    int cx = (startX + x) / 2;
    int cy = (startY + y) / 2;

    if (rx > 0 && ry > 0)
    {
        // Explicitly route to scratchpad
        Ellipse(cx, cy, rx, ry, r, g, b, a, size).draw(manager, &LayerManager::setPreviewPixel);
        manager.showPreview();
    }
}

void EllipseTool::onRelease(int x, int y, LayerManager &manager)
{
    if (!isDrawing)
        return;

    manager.clearPreview();

    int rx = std::abs(x - startX) / 2;
    int ry = std::abs(y - startY) / 2;
    int cx = (startX + x) / 2;
    int cy = (startY + y) / 2;

    if (rx > 0 && ry > 0)
    {
        manager.beginBatch();
        // Default routing to permanent memory
        Ellipse(cx, cy, rx, ry, r, g, b, a, size).draw(manager);
        manager.endBatch();
    }

    isDrawing = false;
}

// ==========================================
// ERASER TOOL
// ==========================================
void EraserTool::onPress(int x, int y, LayerManager &manager)
{
    manager.clearPreview(); // Clear hover outline when clicking down
    isErasing = true;
    lastX = x;
    lastY = y;

    manager.beginBatch();
    // Draw a completely transparent pixel (Alpha = 0)
    Line(x, y, x, y, 0, 0, 0, 0, size).draw(manager);
    manager.endBatch();
}

void EraserTool::onHover(int x, int y, LayerManager &manager)
{
    if (isErasing)
        return; // Don't show the hover cursor if we're actively erasing
    manager.clearPreview();

    // Draw a RED circle outline to indicate it's an eraser
    CircleOutline(x, y, size, 255, 0, 0, 255).draw(manager, &LayerManager::setPreviewPixel);

    manager.showPreview();
}

void EraserTool::onMove(int x, int y, LayerManager &manager)
{
    if (!isErasing)
        return;

    manager.beginBatch();
    // Erase a stroke from the last position to the current one
    Line(lastX, lastY, x, y, 0, 0, 0, 0, size).draw(manager);
    manager.endBatch();

    lastX = x;
    lastY = y;
}

void EraserTool::onRelease(int x, int y, LayerManager &manager)
{
    isErasing = false;
}