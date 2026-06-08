#include "Tool.h"
#include <iostream>
#include <cmath>

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

    pointHistory.push_back({x, y});
    sumX += x;
    sumY += y;

    int historyCount = static_cast<int>(pointHistory.size());
    int avgX = sumX / historyCount;
    int avgY = sumY / historyCount;

    manager.beginBatch();

    int currentX = avgX;
    int currentY = avgY;

    int startMidX = (prevX + lastX) / 2;
    int startMidY = (prevY + lastY) / 2;
    int endMidX = (lastX + currentX) / 2;
    int endMidY = (lastY + currentY) / 2;

    float dx1 = static_cast<float>(lastX - startMidX);
    float dy1 = static_cast<float>(lastY - startMidY);
    float dist1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
    float dx2 = static_cast<float>(currentX - endMidX);
    float dy2 = static_cast<float>(currentY - endMidY);
    float dist2 = std::sqrt(dx2 * dx2 + dy2 * dy2);

    float stepDistance = std::max(1.0f, size * 0.25f);
    int steps = std::max(1, static_cast<int>((dist1 + dist2) / stepDistance));

    int minX = x, minY = y, maxX = x, maxY = y;

    int currentSegX = startMidX;
    int currentSegY = startMidY;

    for (int i = 1; i <= steps; ++i)
    {
        float t = static_cast<float>(i) / steps;
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

    manager.addDirtyRect(minX - size, minY - size, (maxX - minX) + size * 2 + 1, (maxY - minY) + size * 2 + 1);
    manager.endBatch();

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
    Line(x0, y0, x1, y1, 0, 0, 0, 0, size).draw(manager);
}
void EraserTool::drawHoverCursor(int x, int y, LayerManager &manager)
{
    CircleOutline(x, y, size, 255, 0, 0, 255).draw(manager, &LayerManager::setPreviewPixel);
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
}

void ShapeTool::onMove(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    if (!isDrawing)
        return;
    if (!shouldProcessMove())
        return;

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

void RectangleTool::drawShapePreview(int sx, int sy, int cx, int cy, LayerManager &manager)
{
    int rx = std::min(sx, cx);
    int ry = std::min(sy, cy);
    int w = std::abs(cx - sx);
    int h = std::abs(cy - sy);

    if (w > 0 && h > 0)
    {
        Rectangle(rx, ry, w, h, r, g, b, a, size).draw(manager, &LayerManager::setPreviewPixel);
        int p = size;
        manager.addPreviewDirtyRect(rx - p, ry - p, w + (p * 2), size + (p * 2));
        manager.addPreviewDirtyRect(rx - p, (ry + h) - size - p, w + (p * 2), size + (p * 2));
        manager.addPreviewDirtyRect(rx - p, ry - p, size + (p * 2), h + (p * 2));
        manager.addPreviewDirtyRect((rx + w) - size - p, ry - p, size + (p * 2), h + (p * 2));
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
        Rectangle(rx, ry, w, h, r, g, b, a, size).draw(manager);
        int p = size;
        manager.addDirtyRect(rx - p, ry - p, w + (p * 2), size + (p * 2));
        manager.addDirtyRect(rx - p, (ry + h) - size - p, w + (p * 2), size + (p * 2));
        manager.addDirtyRect(rx - p, ry - p, size + (p * 2), h + (p * 2));
        manager.addDirtyRect((rx + w) - size - p, ry - p, size + (p * 2), h + (p * 2));
    }
}

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
// FILL TOOL
// ==========================================
void FillTool::onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    manager.clearPreview();

    if (x < 0 || x >= manager.getWidth() || y < 0 || y >= manager.getHeight())
        return;

    uint8_t tr, tg, tb, ta;
    manager.getPixel(x, y, tr, tg, tb, ta);

    if (tr == r && tg == g && tb == b && ta == a)
        return;

    manager.beginBatch();

    std::vector<std::pair<int, int>> stack;
    stack.push_back({x, y});

    int minX = x, minY = y, maxX = x, maxY = y;

    while (!stack.empty())
    {
        auto [cx, cy] = stack.back();
        stack.pop_back();

        uint8_t cr, cg, cb, ca;
        int lx = cx;

        while (lx >= 0)
        {
            manager.getPixel(lx, cy, cr, cg, cb, ca);
            if (cr != tr || cg != tg || cb != tb || ca != ta)
                break;
            lx--;
        }
        lx++;

        int rx = cx;
        while (rx < manager.getWidth())
        {
            manager.getPixel(rx, cy, cr, cg, cb, ca);
            if (cr != tr || cg != tg || cb != tb || ca != ta)
                break;
            rx++;
        }
        rx--;

        minX = std::min(minX, lx);
        maxX = std::max(maxX, rx);
        minY = std::min(minY, cy);
        maxY = std::max(maxY, cy);

        bool spanAbove = false;
        bool spanBelow = false;

        for (int i = lx; i <= rx; ++i)
        {
            manager.setPixel(i, cy, r, g, b, a);

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
                    spanAbove = false;
            }

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
                    spanBelow = false;
            }
        }
    }

    manager.addDirtyRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
    manager.endBatch();
}

// ==========================================
// TOOL DECORATORS
// ==========================================

PressureSizeDecorator::PressureSizeDecorator(std::unique_ptr<Tool> tool)
    : ToolDecorator(std::move(tool)), originalBaseSize(5) {}

void PressureSizeDecorator::onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    if (!wrappedTool)
        return;

    // 1. Lock in the size that the UI slider is currently set to
    originalBaseSize = wrappedTool->size;

    // 2. Scale the wrapped tool's size by the physical pressure
    wrappedTool->size = std::max(1, static_cast<int>(originalBaseSize * pressure));

    // 3. Delegate execution
    wrappedTool->onPress(x, y, pressure, tiltX, tiltY, manager);
}

void PressureSizeDecorator::onMove(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    if (!wrappedTool)
        return;

    wrappedTool->size = std::max(1, static_cast<int>(originalBaseSize * pressure));
    wrappedTool->onMove(x, y, pressure, tiltX, tiltY, manager);
}

void PressureSizeDecorator::onRelease(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    if (!wrappedTool)
        return;

    wrappedTool->onRelease(x, y, pressure, tiltX, tiltY, manager);

    // 4. SNAP BACK! Restore the original UI size.
    wrappedTool->size = originalBaseSize;
}

ConeGrowthDecorator::ConeGrowthDecorator(std::unique_ptr<Tool> tool, float rate, float factor)
    : ToolDecorator(std::move(tool)), growthRate(rate), growthFactor(factor), originalBaseSize(5) {}

void ConeGrowthDecorator::onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    if (!wrappedTool)
        return;

    originalBaseSize = wrappedTool->size;
    calculatedMaxSize = static_cast<float>(originalBaseSize) * growthFactor;
    currentDynamicSize = 1.0f;

    wrappedTool->size = static_cast<int>(currentDynamicSize);
    rawLastX = x;
    rawLastY = y;

    wrappedTool->onPress(x, y, pressure, tiltX, tiltY, manager);
}

void ConeGrowthDecorator::onMove(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    if (!wrappedTool)
        return;

    float dist = std::sqrt(std::pow(x - rawLastX, 2) + std::pow(y - rawLastY, 2));
    currentDynamicSize += (dist * growthRate);
    currentDynamicSize = std::min(currentDynamicSize, calculatedMaxSize);

    wrappedTool->size = static_cast<int>(currentDynamicSize);
    rawLastX = x;
    rawLastY = y;

    wrappedTool->onMove(x, y, pressure, tiltX, tiltY, manager);
}

void ConeGrowthDecorator::onRelease(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    if (!wrappedTool)
        return;

    wrappedTool->onRelease(x, y, pressure, tiltX, tiltY, manager);
    wrappedTool->size = originalBaseSize;
}

// ==========================================
// MASK BRUSH TOOL
// ==========================================

MaskBrushTool::MaskBrushTool(std::shared_ptr<MaskLayer> mask) : brushTip(mask) {}

void MaskBrushTool::drawLineSegment(int x0, int y0, int x1, int y1, LayerManager &manager)
{
    if (!brushTip)
        return;

    float dx = x1 - x0;
    float dy = y1 - y0;
    float dist = std::sqrt(dx * dx + dy * dy);

    float spacing = std::max(1.0f, static_cast<float>(size * 0.15f));
    int steps = std::max(1, static_cast<int>(dist / spacing));

    for (int i = 0; i <= steps; ++i)
    {
        float t = static_cast<float>(i) / steps;
        int cx = x0 + (dx * t);
        int cy = y0 + (dy * t);

        stampMask(cx, cy, manager);
    }
}

void MaskBrushTool::stampMask(int cx, int cy, LayerManager &manager)
{
    int stampDiameter = size;
    if (stampDiameter <= 0)
        stampDiameter = 1;

    int offset = stampDiameter / 2;
    int startX = cx - offset;
    int startY = cy - offset;

    int maskW = brushTip->width;
    int maskH = brushTip->height;

    for (int y = 0; y < stampDiameter; ++y)
    {
        for (int x = 0; x < stampDiameter; ++x)
        {
            int maskX = (x * maskW) / stampDiameter;
            int maskY = (y * maskH) / stampDiameter;

            uint8_t mr, mg, mb, ma;
            brushTip->getPixel(maskX, maskY, mr, mg, mb, ma);

            if (ma > 0)
            {
                int canvasX = startX + x;
                int canvasY = startY + y;

                float normalizedMaskAlpha = ma / 255.0f;
                float normalizedToolAlpha = a / 255.0f;
                uint8_t finalAlpha = static_cast<uint8_t>(normalizedMaskAlpha * normalizedToolAlpha * 255.0f);

                uint8_t bgR, bgG, bgB, bgA;
                manager.getPixel(canvasX, canvasY, bgR, bgG, bgB, bgA);

                float alphaF = finalAlpha / 255.0f;
                float invAlpha = 1.0f - alphaF;

                uint8_t outR = static_cast<uint8_t>((r * alphaF) + (bgR * invAlpha));
                uint8_t outG = static_cast<uint8_t>((g * alphaF) + (bgG * invAlpha));
                uint8_t outB = static_cast<uint8_t>((b * alphaF) + (bgB * invAlpha));

                uint8_t outA = std::min(255, bgA + finalAlpha);

                manager.setPixel(canvasX, canvasY, outR, outG, outB, outA);
            }
        }
    }
}

void MaskBrushTool::drawHoverCursor(int x, int y, LayerManager &manager)
{
    CircleOutline(x, y, size, 50, 50, 50, 255).draw(manager, &LayerManager::setPreviewPixel);
}