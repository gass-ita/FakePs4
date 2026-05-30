#include "Tool.h"

// ==========================================
// BRUSH TOOL
// ==========================================
void BrushTool::onPress(int x, int y, LayerManager &manager)
{
    isDrawing = true;
    lastX = x;
    lastY = y;

    manager.beginBatch();
    Line(x, y, x, y, 255, 0, 0).draw(manager); // Red dot
    manager.endBatch();
}

void BrushTool::onMove(int x, int y, LayerManager &manager)
{
    if (!isDrawing)
        return;

    manager.beginBatch();
    Line(lastX, lastY, x, y, 255, 0, 0).draw(manager); // Red stroke
    manager.endBatch();

    lastX = x;
    lastY = y;
}

void BrushTool::onRelease(int x, int y, LayerManager &manager)
{
    isDrawing = false;
}

// ==========================================
// RECTANGLE TOOL (Stamps a 100x50 box)
// ==========================================
void RectangleTool::onPress(int x, int y, LayerManager &manager)
{
    manager.beginBatch();
    Rectangle(x, y, 100, 50, 0, 0, 255).draw(manager); // Blue rectangle
    manager.endBatch();
}

void RectangleTool::onMove(int x, int y, LayerManager &manager)
{
    // A simple stamp tool does nothing on move!
}

void RectangleTool::onRelease(int x, int y, LayerManager &manager)
{
    // Do nothing
}

void EraserTool::onPress(int x, int y, LayerManager &manager)
{
    isErasing = true;
    lastX = x;
    lastY = y;

    manager.beginBatch();
    // Draw a completely transparent pixel (Alpha = 0)
    Line(x, y, x, y, 0, 0, 0, 0).draw(manager);
    manager.endBatch();
}

void EraserTool::onMove(int x, int y, LayerManager &manager)
{
    if (!isErasing)
        return;

    manager.beginBatch();
    // Erase a stroke from the last position to the current one
    Line(lastX, lastY, x, y, 0, 0, 0, 0).draw(manager);
    manager.endBatch();

    lastX = x;
    lastY = y;
}

void EraserTool::onRelease(int x, int y, LayerManager &manager)
{
    isErasing = false;
}