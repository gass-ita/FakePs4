#include "Tool.h"
#include <iostream>
#include <cmath>

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

    // add the dirty rect to the batch list
    int minX = x - size;
    int minY = y - size;
    int maxX = x + size;
    int maxY = y + size;
    manager.addDirtyRect(minX, minY, maxX - minX + 1, maxY - minY + 1);

    manager.endBatch();
}

void BrushTool::onHover(int x, int y, LayerManager &manager)
{
    if (isDrawing)
        return; // Don't show the hover cursor if we're actively drawing

    manager.clearPreview();

    CircleOutline(x, y, size, 50, 50, 50, 255).draw(manager, &LayerManager::setPreviewPixel);

    // --- SUBMIT PREVIEW RECTANGLE ---
    int pad = size + 2; // +2 guarantees anti-aliased edges fit inside the box
    manager.addPreviewDirtyRect(x - pad, y - pad, pad * 2, pad * 2);
    // --------------------------------

    manager.showPreview();
}
void BrushTool::onMove(int x, int y, LayerManager &manager)
{
    if (!isDrawing)
        return;

    manager.beginBatch();
    Line(lastX, lastY, x, y, r, g, b, a, size).draw(manager);

    // add the dirty rect to the batch list
    int minX = std::min(lastX, x) - size;
    int minY = std::min(lastY, y) - size;
    int maxX = std::max(lastX, x) + size;
    int maxY = std::max(lastY, y) + size;
    manager.addDirtyRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
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

    // 1. Wipes the old frame
    manager.clearPreview();

    int rectX = std::min(startX, x);
    int rectY = std::min(startY, y);
    int width = std::abs(x - startX);
    int height = std::abs(y - startY);

    if (width > 0 && height > 0)
    {
        // 2. Draw to scratchpad
        Rectangle(rectX, rectY, width, height, r, g, b, a, size).draw(manager, &LayerManager::setPreviewPixel);

        // 3. Submit the 4 Hollow Walls to the Preview Tracker!
        int pad = size;
        manager.addPreviewDirtyRect(rectX - pad, rectY - pad, width + (pad * 2), size + (pad * 2));
        manager.addPreviewDirtyRect(rectX - pad, (rectY + height) - size - pad, width + (pad * 2), size + (pad * 2));
        manager.addPreviewDirtyRect(rectX - pad, rectY - pad, size + (pad * 2), height + (pad * 2));
        manager.addPreviewDirtyRect((rectX + width) - size - pad, rectY - pad, size + (pad * 2), height + (pad * 2));

        // 4. Force Qt to repaint those specific walls
        manager.showPreview();
    }
}

void RectangleTool::onRelease(int x, int y, LayerManager &manager)
{
    if (!isDrawing)
        return;

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
        manager.addDirtyRect(rectX - size, rectY - size, width + (size * 2), size + (size * 2));
        manager.addDirtyRect(rectX - size, (rectY + height) - size - size, width + (size * 2), size + (size * 2));
        manager.addDirtyRect(rectX - size, rectY - size, size + (size * 2), height + (size * 2));
        manager.addDirtyRect((rectX + width) - size - size, rectY - size, size + (size * 2), height + (size * 2));
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
        Ellipse(cx, cy, rx, ry, r, g, b, a, size).draw(manager, &LayerManager::setPreviewPixel);

        // --- THE O(PERIMETER) HOLLOW OPTIMIZATION ---
        int pad = size + 2;
        int rectW = (rx * 2) + (pad * 2);
        int rectH = (ry * 2) + (pad * 2);

        // 1. If it's a small ellipse, a solid box is actually faster
        if (rectW < 512 && rectH < 512)
        {
            int rectX = (cx - rx) - pad;
            int rectY = (cy - ry) - pad;
            manager.addPreviewDirtyRect(rectX, rectY, rectW, rectH);
        }
        // 2. If it's massive, trace the hollow ring using Sine/Cosine!
        else
        {
            int steps = std::max(rx, ry) * 2; // Ensure we trace enough points so no gaps exist
            for (int i = 0; i < steps; ++i)
            {
                float angle = (i * 6.2831853f) / steps; // 6.283 is 2 * Pi
                int px = cx + (rx * std::cos(angle));
                int py = cy + (ry * std::sin(angle));
                manager.addPreviewDirtyRect(px - pad, py - pad, pad * 2, pad * 2);
            }
        }
        // --------------------------------------------

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
        Ellipse(cx, cy, rx, ry, r, g, b, a, size).draw(manager);

        // --- SAME OPTIMIZATION FOR PERMANENT BURN-IN ---
        int pad = size + 2;
        int rectW = (rx * 2) + (pad * 2);
        int rectH = (ry * 2) + (pad * 2);

        if (rectW < 512 && rectH < 512)
        {
            int rectX = (cx - rx) - pad;
            int rectY = (cy - ry) - pad;
            manager.addDirtyRect(rectX, rectY, rectW, rectH);
        }
        else
        {
            int steps = std::max(rx, ry) * 2;
            for (int i = 0; i < steps; ++i)
            {
                float angle = (i * 6.2831853f) / steps;
                int px = cx + (rx * std::cos(angle));
                int py = cy + (ry * std::sin(angle));
                manager.addDirtyRect(px - pad, py - pad, pad * 2, pad * 2);
            }
        }

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
    int minX = std::min(lastX, x) - size;
    int minY = std::min(lastY, y) - size;
    int maxX = std::max(lastX, x) + size;
    int maxY = std::max(lastY, y) + size;
    manager.addDirtyRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
    manager.endBatch();
}

void EraserTool::onHover(int x, int y, LayerManager &manager)
{
    if (isErasing)
        return; // Don't show the hover cursor if we're actively erasing

    manager.clearPreview();

    // Draw a RED circle outline to indicate it's an eraser
    CircleOutline(x, y, size, 255, 0, 0, 255).draw(manager, &LayerManager::setPreviewPixel);

    // --- SUBMIT PREVIEW RECTANGLE ---
    int pad = size + 2;
    manager.addPreviewDirtyRect(x - pad, y - pad, pad * 2, pad * 2);
    // --------------------------------

    manager.showPreview();
}

void EraserTool::onMove(int x, int y, LayerManager &manager)
{
    if (!isErasing)
        return;

    manager.beginBatch();
    // Erase a stroke from the last position to the current one
    Line(lastX, lastY, x, y, 0, 0, 0, 0, size).draw(manager);
    int minX = std::min(lastX, x) - size;
    int minY = std::min(lastY, y) - size;
    int maxX = std::max(lastX, x) + size;
    int maxY = std::max(lastY, y) + size;
    manager.addDirtyRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
    manager.endBatch();

    lastX = x;
    lastY = y;
}

void EraserTool::onRelease(int x, int y, LayerManager &manager)
{
    isErasing = false;
}