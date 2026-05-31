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
    Line(x, y, x, y, r, g, b, a, size).draw(manager); // Red dot
    manager.endBatch();
}

void BrushTool::onMove(int x, int y, LayerManager &manager)
{
    if (!isDrawing)
        return;

    manager.beginBatch();
    Line(lastX, lastY, x, y, r, g, b, a, size).draw(manager); // Red stroke
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
    Rectangle(x, y, 100, 50, r, g, b, a, size).draw(manager);
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

// ==========================================
// ELLIPSE TOOL (Stamps an 80x50 ellipse)
// ==========================================
void EllipseTool::onPress(int x, int y, LayerManager &manager)
{
    manager.beginBatch();
    // Center at (x,y), x-radius 80, y-radius 50, Green color, uses slider size for thickness
    Ellipse(x, y, 80, 50, r, g, b, a, size).draw(manager);
    manager.endBatch();
}

void EllipseTool::onMove(int x, int y, LayerManager &manager) {}
void EllipseTool::onRelease(int x, int y, LayerManager &manager) {}

void EraserTool::onPress(int x, int y, LayerManager &manager)
{
    isErasing = true;
    lastX = x;
    lastY = y;

    manager.beginBatch();
    // Draw a completely transparent pixel (Alpha = 0)
    Line(x, y, x, y, 0, 0, 0, 0, size).draw(manager);
    manager.endBatch();
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