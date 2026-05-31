#include "Tool.h"
#include <iostream>

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

void BrushTool::onHover(int x, int y, LayerManager &manager)
{
    
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
// RECTANGLE TOOL (Click and Drag)
// ==========================================
void RectangleTool::onPress(int x, int y, LayerManager &manager)
{
    startX = x;
    startY = y;
    isDrawing = true;
}

void RectangleTool::onMove(int x, int y, LayerManager &manager)
{
  
}

void RectangleTool::onRelease(int x, int y, LayerManager &manager)
{
    if (!isDrawing)
        return;


    int rectX = std::min(startX, x);
    int rectY = std::min(startY, y);
    int width = std::abs(x - startX);
    int height = std::abs(y - startY);

    if (width > 0 && height > 0)
    {
        manager.beginBatch();
        // Because preview mode is off, this burns into the real active layer
        Rectangle(rectX, rectY, width, height, this->r, this->g, this->b, this->a, this->size).draw(manager);
        manager.endBatch();
    }

    isDrawing = false;
}

// ==========================================
// ELLIPSE TOOL (Click and Drag)
// ==========================================
void EllipseTool::onPress(int x, int y, LayerManager &manager)
{
    startX = x;
    startY = y;
    isDrawing = true;
}

void EllipseTool::onMove(int x, int y, LayerManager &manager)
{
    
}

void EllipseTool::onRelease(int x, int y, LayerManager &manager)
{
    if (!isDrawing)
        return;

    

    int rx = std::abs(x - startX) / 2;
    int ry = std::abs(y - startY) / 2;
    int cx = (startX + x) / 2;
    int cy = (startY + y) / 2;

    if (rx > 0 && ry > 0)
    {
        manager.beginBatch();
        // 2. Because preview mode is false, this burns into the REAL active layer
        Ellipse(cx, cy, rx, ry, this->r, this->g, this->b, this->a, this->size).draw(manager);
        manager.endBatch();
    }

    isDrawing = false;
}

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

void EraserTool::onHover(int x, int y, LayerManager &manager)
{
    
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