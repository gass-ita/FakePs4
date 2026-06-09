#include "Tool.h"
#include <iostream>
#include <cmath>
// for round

void ColorPickerTool::onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    uint8_t r, g, b, a;
    manager.getPixel(x, y, r, g, b, a);
    manager.setToolColor(r, g, b, a);
}

// ==========================================
// STROKE TOOL (Base Class Boilerplate)
// ==========================================
void StrokeTool::onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    manager.clearPreview();
    isDrawing = true;

    // Reset the Stabilizer History!
    pointHistory.clear();
    sumX = 0;
    sumY = 0;

    for (int i = 0; i < smoothingWindow; ++i)
    {
        pointHistory.push_back({x, y});
        sumX += x;
        sumY += y;
    }

    lastX = x;
    lastY = y;
    prevX = lastX;
    prevY = lastY;

    manager.beginBatch();
    drawLineSegment(x, y, x, y, manager);
    manager.addDirtyRect(x - size, y - size, size * 2 + 1, size * 2 + 1);
    manager.endBatch();
}

void StrokeTool::onMove(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    if (!isDrawing)
        return;

    sumX -= pointHistory.front().first;
    sumY -= pointHistory.front().second;
    pointHistory.pop_front();

    // --- 1. STABILIZER (MOVING AVERAGE) ---
    pointHistory.push_back({x, y});
    sumX += x;
    sumY += y;

    int historyCount = static_cast<int>(pointHistory.size());
    int avgX = sumX / historyCount;
    int avgY = sumY / historyCount;

    // --------------------------------------

    manager.beginBatch();

    int currentX = avgX;
    int currentY = avgY;

    // 2. Midpoint calculation
    int startMidX = (prevX + lastX) / 2;
    int startMidY = (prevY + lastY) / 2;
    int endMidX = (lastX + currentX) / 2;
    int endMidY = (lastY + currentY) / 2;

    // 3. Distance calculation (still using float for std::sqrt, but casting result to int)
    float dx1 = static_cast<float>(lastX - startMidX);
    float dy1 = static_cast<float>(lastY - startMidY);
    float dist1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
    float dx2 = static_cast<float>(currentX - endMidX);
    float dy2 = static_cast<float>(currentY - endMidY);
    float dist2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
    // ==========================================
    // ADAPTIVE BEZIER STEPPING (The 10x Speedup)
    // ==========================================
    // Thin brushes need tight steps. Fat brushes can step massive distances
    // because their huge rounded caps overlap and hide the gaps!
    float stepDistance = std::max(1.0f, size * 0.25f);
    int steps = std::max(1, static_cast<int>((dist1 + dist2) / stepDistance));
    // ==========================================

    int minX = x, minY = y, maxX = x, maxY = y;

    int currentSegX = startMidX;
    int currentSegY = startMidY;

    // 4. Step along the curve
    for (int i = 1; i <= steps; ++i)
    {
        float t = static_cast<float>(i) / steps; // 't' remains a float percentage
        int nextX, nextY;

        calculateBezierPoint(t, startMidX, lastX, endMidX, nextX);
        calculateBezierPoint(t, startMidY, lastY, endMidY, nextY);

        drawLineSegment(currentSegX, currentSegY, nextX, nextY, manager);

        minX = std::min({minX, currentSegX, nextX});
        minY = std::min({minY, currentSegY, nextY});
        maxX = std::max({maxX, currentSegX, nextX});
        maxY = std::max({maxY, currentSegY, nextY});

        currentSegX = nextX;
        currentSegY = nextY;
    }

    // 5. Submit Dirty Rect
    manager.addDirtyRect(minX - size, minY - size, (maxX - minX) + size * 2 + 1, (maxY - minY) + size * 2 + 1);
    manager.endBatch();

    // 6. Shift history
    prevX = lastX;
    prevY = lastY;
    lastX = currentX;
    lastY = currentY;
}

void StrokeTool::onRelease(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    isDrawing = false;
}

void StrokeTool::onHover(int x, int y, LayerManager &manager)
{
    if (isDrawing)
        return;

    manager.clearPreview();
    drawHoverCursor(x, y, manager);

    int pad = size + 2;
    manager.addPreviewDirtyRect(x - pad, y - pad, pad * 2, pad * 2);
    manager.showPreview();
}

// --- CONCRETE STROKE IMPLEMENTATIONS ---

void BrushTool::drawLineSegment(int x0, int y0, int x1, int y1, LayerManager &manager)
{
    Line(x0, y0, x1, y1, r, g, b, a, size).draw(manager);
}
void BrushTool::drawHoverCursor(int x, int y, LayerManager &manager)
{
    CircleOutline(x, y, size, 50, 50, 50, 255).draw(manager, &LayerManager::setPreviewPixel);
}

void EraserTool::drawLineSegment(int x0, int y0, int x1, int y1, LayerManager &manager)
{
    Line(x0, y0, x1, y1, 0, 0, 0, 0, size).draw(manager); // Transparent!
}
void EraserTool::drawHoverCursor(int x, int y, LayerManager &manager)
{
    CircleOutline(x, y, size, 255, 0, 0, 255).draw(manager, &LayerManager::setPreviewPixel); // Red!
}

void SprayTool::drawLineSegment(int x0, int y0, int x1, int y1, LayerManager &manager)
{
    SprayShape(x1, y1, r, g, b, a, size, 0.5).draw(manager);
}

void SprayTool::drawHoverCursor(int x, int y, LayerManager &manager)
{
    CircleOutline(x, y, size, 50, 50, 50, 255).draw(manager, &LayerManager::setPreviewPixel);
}

// ==========================================
// SHAPE TOOL (Base Class Boilerplate)
// ==========================================
void ShapeTool::onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    manager.clearPreview();
    startX = x;
    startY = y;
    isDrawing = true;

    static int lastX = -1;
    static int lastY = -1;
    lastX = x;
    lastY = y;
}

void ShapeTool::onMove(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    if (!isDrawing)
        return;

    static int lastX = -1;
    static int lastY = -1;
    if (x == lastX && y == lastY)
        return;

    if (!shouldProcessMove())
        return;

    lastX = x;
    lastY = y;

    manager.clearPreview();
    drawShapePreview(startX, startY, x, y, manager);
    manager.showPreview();
}

void ShapeTool::onRelease(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    if (!isDrawing)
        return;

    manager.clearPreview();
    manager.beginBatch();
    drawShapeFinal(startX, startY, x, y, manager);
    manager.endBatch();

    isDrawing = false;
}

// --- CONCRETE SHAPE IMPLEMENTATIONS ---

// 1. RECTANGLE
void RectangleTool::drawShapePreview(int sx, int sy, int cx, int cy, LayerManager &manager)
{
    int rx = std::min(sx, cx);
    int ry = std::min(sy, cy);
    int w = std::abs(cx - sx);
    int h = std::abs(cy - sy);

    if (w > 0 && h > 0)
    {
        // 1. Instantiating and rendering the 4 lines directly on the stack
        Line(rx, ry, rx + w, ry, r, g, b, a, size).draw(manager, &LayerManager::setPreviewPixel);         // Top Edge
        Line(rx + w, ry, rx + w, ry + h, r, g, b, a, size).draw(manager, &LayerManager::setPreviewPixel); // Right Edge
        Line(rx + w, ry + h, rx, ry + h, r, g, b, a, size).draw(manager, &LayerManager::setPreviewPixel); // Bottom Edge
        Line(rx, ry + h, rx, ry, r, g, b, a, size).draw(manager, &LayerManager::setPreviewPixel);         // Left Edge

        // 2. Submit the single optimized bounding box for the entire preview rectangle
        int pad = size + 2;
        manager.addPreviewDirtyRect(rx - pad, ry - pad, w + (pad * 2), h + (pad * 2));
    }
}

void RectangleTool::drawShapeFinal(int sx, int sy, int cx, int cy, LayerManager &manager)
{
    int rx = std::min(sx, cx);
    int ry = std::min(sy, cy);
    int w = std::abs(cx - sx);
    int h = std::abs(cy - sy);

    if (w > 0 && h > 0)
    {
        // 1. Permanently draw the 4 lines directly to the target layer memory
        Line(rx, ry, rx + w, ry, r, g, b, a, size).draw(manager);         // Top Edge
        Line(rx + w, ry, rx + w, ry + h, r, g, b, a, size).draw(manager); // Right Edge
        Line(rx + w, ry + h, rx, ry + h, r, g, b, a, size).draw(manager); // Bottom Edge
        Line(rx, ry + h, rx, ry, r, g, b, a, size).draw(manager);         // Left Edge

        // 2. Submit the single optimized bounding box to update the permanent viewport tiles
        int pad = size + 2;
        manager.addDirtyRect(rx - pad, ry - pad, w + (pad * 2), h + (pad * 2));
    }
}

// 2. ELLIPSE
void EllipseTool::drawShapePreview(int sx, int sy, int cx, int cy, LayerManager &manager)
{
    int rx = std::abs(cx - sx) / 2;
    int ry = std::abs(cy - sy) / 2;
    int cenX = (sx + cx) / 2;
    int cenY = (sy + cy) / 2;

    if (rx > 0 && ry > 0)
    {
        Ellipse(cenX, cenY, rx, ry, r, g, b, a, size).draw(manager, &LayerManager::setPreviewPixel);

        int pad = size + 2;
        int rectW = (rx * 2) + (pad * 2);
        int rectH = (ry * 2) + (pad * 2);

        if (rectW < 512 && rectH < 512)
        {
            manager.addPreviewDirtyRect((cenX - rx) - pad, (cenY - ry) - pad, rectW, rectH);
        }
        else
        {
            int steps = std::max(rx, ry) * 2;
            for (int i = 0; i < steps; ++i)
            {
                float angle = (i * 6.2831853f) / steps;
                int px = cenX + (rx * std::cos(angle));
                int py = cenY + (ry * std::sin(angle));
                manager.addPreviewDirtyRect(px - pad, py - pad, pad * 2, pad * 2);
            }
        }
    }
}

void EllipseTool::drawShapeFinal(int sx, int sy, int cx, int cy, LayerManager &manager)
{
    // Exactly the same logic, but routing to the permanent memory and addDirtyRect!
    int rx = std::abs(cx - sx) / 2;
    int ry = std::abs(cy - sy) / 2;
    int cenX = (sx + cx) / 2;
    int cenY = (sy + cy) / 2;

    if (rx > 0 && ry > 0)
    {
        Ellipse(cenX, cenY, rx, ry, r, g, b, a, size).draw(manager);

        int pad = size + 2;
        int rectW = (rx * 2) + (pad * 2);
        int rectH = (ry * 2) + (pad * 2);

        if (rectW < 512 && rectH < 512)
        {
            manager.addDirtyRect((cenX - rx) - pad, (cenY - ry) - pad, rectW, rectH);
        }
        else
        {
            int steps = std::max(rx, ry) * 2;
            for (int i = 0; i < steps; ++i)
            {
                float angle = (i * 6.2831853f) / steps;
                int px = cenX + (rx * std::cos(angle));
                int py = cenY + (ry * std::sin(angle));
                manager.addDirtyRect(px - pad, py - pad, pad * 2, pad * 2);
            }
        }
    }
}

// ==========================================
// FILL TOOL (Scanline Span Algorithm)
// ==========================================
void FillTool::onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    manager.clearPreview();

    // 1. Boundary check
    if (x < 0 || x >= manager.getWidth() || y < 0 || y >= manager.getHeight())
        return;

    // 2. Sample the "Target Color" we want to overwrite
    uint8_t tr, tg, tb, ta;
    manager.getPixel(x, y, tr, tg, tb, ta);

    // Failsafe: If the pixel is already the exact color we are holding, do nothing!
    if (tr == r && tg == g && tb == b && ta == a)
        return;

    manager.beginBatch();

    // The stack holds the X, Y coordinates of rows we still need to process
    std::vector<std::pair<int, int>> stack;
    stack.push_back({x, y});

    // Track the dirty bounding box so we can dispatch the exact tiles later
    int minX = x, minY = y, maxX = x, maxY = y;

    while (!stack.empty())
    {
        auto [cx, cy] = stack.back();
        stack.pop_back();

        uint8_t cr, cg, cb, ca;
        int lx = cx;

        // 3. Scan Left to find the wall
        while (lx >= 0)
        {
            manager.getPixel(lx, cy, cr, cg, cb, ca);
            if (cr != tr || cg != tg || cb != tb || ca != ta)
                break;
            lx--;
        }
        lx++; // Step back to the valid pixel

        // 4. Scan Right to find the other wall
        int rx = cx;
        while (rx < manager.getWidth())
        {
            manager.getPixel(rx, cy, cr, cg, cb, ca);
            if (cr != tr || cg != tg || cb != tb || ca != ta)
                break;
            rx++;
        }
        rx--;

        // Update our dirty bounding box
        minX = std::min(minX, lx);
        maxX = std::max(maxX, rx);
        minY = std::min(minY, cy);
        maxY = std::max(maxY, cy);

        bool spanAbove = false;
        bool spanBelow = false;

        // 5. Burn the horizontal line into memory and check above/below for more space
        for (int i = lx; i <= rx; ++i)
        {
            manager.setPixel(i, cy, r, g, b, a);

            // Check the row ABOVE
            if (cy > 0)
            {
                manager.getPixel(i, cy - 1, cr, cg, cb, ca);
                bool match = (cr == tr && cg == tg && cb == tb && ca == ta);

                if (match && !spanAbove)
                {
                    stack.push_back({i, cy - 1});
                    spanAbove = true;
                }
                else if (!match)
                {
                    spanAbove = false;
                }
            }

            // Check the row BELOW
            if (cy < manager.getHeight() - 1)
            {
                manager.getPixel(i, cy + 1, cr, cg, cb, ca);
                bool match = (cr == tr && cg == tg && cb == tb && ca == ta);

                if (match && !spanBelow)
                {
                    stack.push_back({i, cy + 1});
                    spanBelow = true;
                }
                else if (!match)
                {
                    spanBelow = false;
                }
            }
        }
    }

    // 6. Report the final, optimized bounding box to the Tile Cache and Undo System!
    manager.addDirtyRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
    manager.endBatch();
}
// ==========================================
// CONE BRUSH TOOL
// ==========================================

// 1. Initialize the threshold in the constructor
// 1. Initialize the rate and factor
ConeBrushTool::ConeBrushTool(float rate, float factor)
    : growthRate(rate), growthFactor(factor), calculatedMaxSize(0.0f), originalBaseSize(5) {}

void ConeBrushTool::onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    // 1. Save the actual UI slider size
    originalBaseSize = size;

    // 2. Calculate the ceiling for this specific stroke! (e.g., 50 * 2.0 = 100)
    calculatedMaxSize = static_cast<float>(originalBaseSize) * growthFactor;

    // 3. Force the brush to start "really small" (1 pixel)
    currentDynamicSize = 1.0f;

    // 4. Temporarily override the base size so the very first dot is small
    size = static_cast<int>(currentDynamicSize);

    // 5. Track position
    rawLastX = x;
    rawLastY = y;

    // 6. Draw the first dot
    BrushTool::onPress(x, y, pressure, tiltX, tiltY, manager);
}

void ConeBrushTool::onMove(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    if (!isDrawing)
        return;

    // 1. Calculate distance moved
    float dist = std::sqrt(std::pow(x - rawLastX, 2) + std::pow(y - rawLastY, 2));

    // 2. Grow the brush
    currentDynamicSize += (dist * growthRate);

    // 3. Clamp it using the dynamic ceiling we calculated in onPress!
    currentDynamicSize = std::min(currentDynamicSize, calculatedMaxSize);

    // 4. Override base size for the Bezier curves
    size = static_cast<int>(currentDynamicSize);

    // 5. Update tracking
    rawLastX = x;
    rawLastY = y;

    // 6. Draw the curve
    BrushTool::onMove(x, y, pressure, tiltX, tiltY, manager);
}

void ConeBrushTool::onRelease(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    BrushTool::onRelease(x, y, pressure, tiltX, tiltY, manager);

    // SNAP BACK! Restore the original UI size so the hover cursor isn't permanently massive.
    size = originalBaseSize;
}

// ==========================================
// PRESSURE BRUSH TOOL
// ==========================================

PressureBrushTool::PressureBrushTool() : originalBaseSize(5) {}

void PressureBrushTool::onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    // 1. Lock in the size that the UI slider is currently set to
    originalBaseSize = size;

    // 2. Scale the size by the physical pressure (0.0 to 1.0)
    // Example: UI size is 50. Pressure is 0.5 (half). Brush becomes 25px.
    size = std::max(1, static_cast<int>(originalBaseSize * pressure));

    // 3. Let the base class draw the initial dot using this new temporary size
    BrushTool::onPress(x, y, pressure, tiltX, tiltY, manager);
}

void PressureBrushTool::onMove(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    if (!isDrawing)
        return;

    // 1. Continually update the size based on how hard they are pressing right now
    size = std::max(1, static_cast<int>(originalBaseSize * pressure));

    // 2. Pass control down to the StrokeTool Bezier math to draw the curve segments
    BrushTool::onMove(x, y, pressure, tiltX, tiltY, manager);
}

void PressureBrushTool::onRelease(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    // 1. Finish the stroke
    BrushTool::onRelease(x, y, pressure, tiltX, tiltY, manager);

    // 2. SNAP BACK! Restore the original UI size.
    // This ensures your UI slider stays accurate and the hover cursor goes back to normal.
    size = originalBaseSize;
}