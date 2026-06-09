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
    if (!shouldProcessMove())
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
    lastX = x;
    lastY = y;

    isDrawing = true;
}

void ShapeTool::onMove(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    if (!isDrawing)
        return;
    if (x == lastX && y == lastY)
        return;
    if (!shouldProcessMove())
        return;

    lastX = x;
    lastY = y;

    // Clear only the tiles touched by the PREVIOUS preview frame.
    // This avoids iterating the full dirty tile set for large shapes.
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

    lastX = -1;
    lastY = -1;

    isDrawing = false;
}

// --- CONCRETE SHAPE IMPLEMENTATIONS ---

void RectangleTool::drawShapePreview(int sx, int sy, int cx, int cy, LayerManager &manager)
{
    int rx = std::min(sx, cx);
    int ry = std::min(sy, cy);
    int w = std::abs(cx - sx);
    int h = std::abs(cy - sy);

    if (w <= 0 || h <= 0)
        return;

    Rectangle(rx, ry, w, h, r, g, b, a, size).draw(manager, &LayerManager::setPreviewPixel);

    // One bounding-box dirty rect instead of four separate edge rects.
    // The tile system deduplicates automatically, so this is never slower
    // and avoids the four addPreviewDirtyRect calls and their grid math.
    int p = size + 1;
    manager.addPreviewDirtyRect(rx - p, ry - p, w + p * 2, h + p * 2);
}
void RectangleTool::drawShapeFinal(int sx, int sy, int cx, int cy, LayerManager &manager)
{
    int rx = std::min(sx, cx);
    int ry = std::min(sy, cy);
    int w = std::abs(cx - sx);
    int h = std::abs(cy - sy);

    if (w <= 0 || h <= 0)
        return;

    Rectangle(rx, ry, w, h, r, g, b, a, size).draw(manager);
    int p = size + 1;
    manager.addDirtyRect(rx - p, ry - p, w + p * 2, h + p * 2);
}

void EllipseTool::drawShapePreview(int sx, int sy, int cx, int cy, LayerManager &manager)
{
    int rx = std::abs(cx - sx) / 2;
    int ry = std::abs(cy - sy) / 2;
    int cenX = (sx + cx) / 2;
    int cenY = (sy + cy) / 2;

    if (rx <= 0 || ry <= 0)
        return;

    Ellipse(cenX, cenY, rx, ry, r, g, b, a, size).draw(manager, &LayerManager::setPreviewPixel);

    // Replace the 512-threshold branch and the angle-loop entirely.
    // One bounding box is always correct and never slower than the
    // angle-loop (which did addPreviewDirtyRect up to max(rx,ry)*2 times).
    int pad = size + 2;
    manager.addPreviewDirtyRect(cenX - rx - pad, cenY - ry - pad,
                                rx * 2 + pad * 2,
                                ry * 2 + pad * 2);
}

void EllipseTool::drawShapeFinal(int sx, int sy, int cx, int cy, LayerManager &manager)
{
    int rx = std::abs(cx - sx) / 2;
    int ry = std::abs(cy - sy) / 2;
    int cenX = (sx + cx) / 2;
    int cenY = (sy + cy) / 2;

    if (rx <= 0 || ry <= 0)
        return;

    Ellipse(cenX, cenY, rx, ry, r, g, b, a, size).draw(manager);
    int pad = size + 2;
    manager.addDirtyRect(cenX - rx - pad, cenY - ry - pad,
                         rx * 2 + pad * 2,
                         ry * 2 + pad * 2);
}
// ==========================================
// FILL TOOL
// ==========================================
void FillTool::onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    const int W = manager.getWidth();
    const int H = manager.getHeight();

    if (x < 0 || x >= W || y < 0 || y >= H)
        return;

    // Sample the target color at the click point
    uint8_t tr, tg, tb, ta;
    manager.getPixel(x, y, tr, tg, tb, ta);

    // If already the fill color, do nothing
    if (tr == r && tg == g && tb == b && ta == a)
        return;

    manager.beginBatch();

    // Visited bitset: one bit per pixel, zero-initialised.
    // Uses ~26 KB for 1920×1080 — trivial stack/heap cost.
    std::vector<bool> visited(W * H, false);

    int minX = x, maxX = x, minY = y, maxY = y;

    std::vector<std::pair<int, int>> stack;
    stack.reserve(1024);
    stack.push_back({x, y});
    visited[y * W + x] = true;

    while (!stack.empty())
    {
        auto [cx, cy] = stack.back();
        stack.pop_back();

        // Scan left
        int lx = cx;
        while (lx > 0)
        {
            uint8_t cr, cg, cb, ca;
            manager.getPixel(lx - 1, cy, cr, cg, cb, ca);
            if (cr != tr || cg != tg || cb != tb || ca != ta)
                break;
            --lx;
            // Mark visited immediately so it's never re-seeded
            if (!visited[cy * W + lx])
                visited[cy * W + lx] = true;
        }

        // Scan right
        int rx = cx;
        while (rx < W - 1)
        {
            uint8_t cr, cg, cb, ca;
            manager.getPixel(rx + 1, cy, cr, cg, cb, ca);
            if (cr != tr || cg != tg || cb != tb || ca != ta)
                break;
            ++rx;
            if (!visited[cy * W + rx])
                visited[cy * W + rx] = true;
        }

        minX = std::min(minX, lx);
        maxX = std::max(maxX, rx);
        minY = std::min(minY, cy);
        maxY = std::max(maxY, cy);

        bool spanAbove = false;
        bool spanBelow = false;

        for (int i = lx; i <= rx; ++i)
        {
            manager.setPixel(i, cy, r, g, b, a);

            // Seed above — only if not yet visited
            if (cy > 0)
            {
                uint8_t cr, cg, cb, ca;
                manager.getPixel(i, cy - 1, cr, cg, cb, ca);
                bool match = (cr == tr && cg == tg && cb == tb && ca == ta);

                if (match && !spanAbove && !visited[(cy - 1) * W + i])
                {
                    stack.push_back({i, cy - 1});
                    visited[(cy - 1) * W + i] = true;
                    spanAbove = true;
                }
                else if (!match)
                    spanAbove = false;
            }

            // Seed below — only if not yet visited
            if (cy < H - 1)
            {
                uint8_t cr, cg, cb, ca;
                manager.getPixel(i, cy + 1, cr, cg, cb, ca);
                bool match = (cr == tr && cg == tg && cb == tb && ca == ta);

                if (match && !spanBelow && !visited[(cy + 1) * W + i])
                {
                    stack.push_back({i, cy + 1});
                    visited[(cy + 1) * W + i] = true;
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

void MaskBrushTool::onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager)
{
    // Reset the math for a brand new brush stroke
    distanceAccumulator = 0.0f;

    // Let the base class set up the Bezier arrays
    StrokeTool::onPress(x, y, pressure, tiltX, tiltY, manager);
}

std::shared_ptr<MaskLayer> MaskBrushTool::createRoundBrushMask(int diameter, float hardness)
{
    if (diameter < 1)
        diameter = 1;

    auto mask = std::make_shared<MaskLayer>(diameter, diameter, "BrushTip");

    float cx = diameter / 2.0f;
    float cy = diameter / 2.0f;
    float radius = diameter / 2.0f;
    float fadeStartRadius = radius * hardness;

    if (radius - fadeStartRadius < 1.0f)
        fadeStartRadius = std::max(0.0f, radius - 1.0f);

    for (int y = 0; y < diameter; ++y)
    {
        for (int x = 0; x < diameter; ++x)
        {
            float px = x + 0.5f;
            float py = y + 0.5f;
            float dist = std::sqrt(std::pow(px - cx, 2) + std::pow(py - cy, 2));

            uint8_t alpha = 0;

            if (dist <= fadeStartRadius)
            {
                alpha = 255;
            }
            else if (dist < radius)
            {
                float fadeLength = radius - fadeStartRadius;
                float distIntoFade = dist - fadeStartRadius;
                float falloff = 1.0f - (distIntoFade / fadeLength);
                alpha = static_cast<uint8_t>(falloff * 255.0f);
            }

            if (alpha > 0)
                mask->setPixel(x, y, 0, 0, 0, alpha);
        }
    }

    return mask;
}

void MaskBrushTool::drawLineSegment(int x0, int y0, int x1, int y1, LayerManager &manager)
{
    if (!brushTip)
        return;

    float dx = x1 - x0;
    float dy = y1 - y0;
    float dist = std::sqrt(dx * dx + dy * dy);

    // If it is the very first dot of a mouse click, stamp it immediately!
    if (dist == 0.0f && distanceAccumulator == 0.0f)
    {
        stampMask(x0, y0, manager);
        return;
    }

    float spacing = std::max(1.0f, static_cast<float>(size * 0.15f));

    // Track how much of THIS specific micro-segment we have traveled
    float segmentTraveled = 0.0f;

    // While we have built up enough total distance to drop a stamp...
    while ((distanceAccumulator + dist - segmentTraveled) >= spacing)
    {
        // Find out how many pixels into this segment the stamp belongs
        float distanceToNextStamp = spacing - distanceAccumulator;
        segmentTraveled += distanceToNextStamp;

        // Convert that distance into a percentage (0.0 to 1.0) to find the X/Y
        float t = segmentTraveled / dist;
        int cx = x0 + (dx * t);
        int cy = y0 + (dy * t);

        stampMask(cx, cy, manager);

        // We just dropped a stamp, so reset the accumulator!
        distanceAccumulator = 0.0f;
    }

    // Add whatever fractional distance is left over to the accumulator
    // so it flawlessly carries over into the next Bezier segment!
    distanceAccumulator += (dist - segmentTraveled);
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

    // ── Hoist 1: tool alpha is constant for the entire stamp ──────────────
    // Old code recomputed (a / 255.0f) inside the inner loop, every pixel.
    const float toolAlphaNorm = a / 255.0f;

    for (int y = 0; y < stampDiameter; ++y)
    {
        // ── Hoist 2: mask Y lookup is constant for the whole row ──────────
        int maskY = (y * maskH) / stampDiameter;

        for (int x = 0; x < stampDiameter; ++x)
        {
            int maskX = (x * maskW) / stampDiameter;

            uint8_t mr, mg, mb, ma;
            brushTip->getPixel(maskX, maskY, mr, mg, mb, ma);

            if (ma == 0)
                continue; // transparent mask pixel — skip entirely

            int canvasX = startX + x;
            int canvasY = startY + y;

            // ── Opt 3: collapse three float ops into one ───────────────────
            // Old: normalizedMaskAlpha = ma/255  →  normalizedToolAlpha = a/255
            //      finalAlpha = normalizedMaskAlpha * normalizedToolAlpha * 255
            // New: toolAlphaNorm is already divided once; one multiply left.
            const float maskAlphaNorm = ma / 255.0f;
            const uint8_t finalAlpha = static_cast<uint8_t>(maskAlphaNorm * toolAlphaNorm * 255.0f);

            if (finalAlpha == 0)
                continue; // guard for rounding to zero

            // Read the canvas pixel under the stamp
            uint8_t bgR, bgG, bgB, bgA;
            manager.getPixel(canvasX, canvasY, bgR, bgG, bgB, bgA);

            // Alpha-blend: out = src * alpha + dst * (1 - alpha)
            const float alphaF = finalAlpha / 255.0f;
            const float invAlpha = 1.0f - alphaF;

            const uint8_t outR = static_cast<uint8_t>(r * alphaF + bgR * invAlpha);
            const uint8_t outG = static_cast<uint8_t>(g * alphaF + bgG * invAlpha);
            const uint8_t outB = static_cast<uint8_t>(b * alphaF + bgB * invAlpha);
            const uint8_t outA = static_cast<uint8_t>(std::min(255, bgA + finalAlpha));

            manager.setPixel(canvasX, canvasY, outR, outG, outB, outA);
        }
    }
}
void MaskBrushTool::drawHoverCursor(int x, int y, LayerManager &manager)
{
    CircleOutline(x, y, size, 50, 50, 50, 255).draw(manager, &LayerManager::setPreviewPixel);
}