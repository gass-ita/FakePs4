#ifndef TOOL_H
#define TOOL_H
#include "LayerManager.h"
#include "Shape.h" // Assuming this is where Line, Rectangle, Ellipse are
#include <deque>
#include <algorithm>
#include <cmath>

class Tool
{
public:
    int size = 5;
    uint8_t r = 0, g = 0, b = 0, a = 255;

    virtual ~Tool() = default;

    virtual void setSize(int newSize) { size = newSize; }
    virtual void setColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
    {
        r = red;
        g = green;
        b = blue;
        a = alpha;
    }

    virtual void onPress(int x, int y, LayerManager &manager) {}
    virtual void onMove(int x, int y, LayerManager &manager) {}
    virtual void onRelease(int x, int y, LayerManager &manager) {}
    virtual void onHover(int x, int y, LayerManager &manager) {}
};

// ==========================================
// 1. THE STROKE ARCHETYPE
// ==========================================
class StrokeTool : public Tool
{
protected:
    /*
     * ==============================================================================
     * BEZIER CURVE HISTORY (Why we need both 'last' and 'prev')
     * ==============================================================================
     * A Quadratic Bezier curve mathematically requires exactly 3 points to exist:
     * 1. P0: Start Anchor
     * 2. P1: Control Point (Acts as "gravity" to pull the curve)
     * 3. P2: End Anchor
     *
     * To make brush strokes perfectly seamless without jagged corners, we do not draw
     * directly from mouse-point to mouse-point. Instead, we draw from the MIDPOINT of
     * old points to the MIDPOINT of new points.
     *
     * - 'current': The smoothed mouse position right now.
     * - 'last':    The position 1 frame ago (Used as the P1 Control Point).
     * - 'prev':    The position 2 frames ago.
     *
     * We MUST track 'prev' because we need it to calculate the Start Anchor (P0),
     * which is the midpoint between 'prev' and 'last'. This guarantees the new curve
     * starts at the exact same pixel and tangent angle where the previous curve ended,
     * creating the illusion of one continuous, fluid ink stroke.
     */
    bool isDrawing = false;
    int lastX = 0;
    int lastY = 0;
    int prevX = 0;
    int prevY = 0;

    short int smoothingWindow = 12;
    std::deque<std::pair<int, int>> pointHistory;

    // Pure virtual functions that child classes MUST implement
    virtual void drawLineSegment(int x0, int y0, int x1, int y1, LayerManager &manager) = 0;
    virtual void drawHoverCursor(int x, int y, LayerManager &manager) = 0;

public:
    void onPress(int x, int y, LayerManager &manager) override;
    void onMove(int x, int y, LayerManager &manager) override;
    void onRelease(int x, int y, LayerManager &manager) override;
    void onHover(int x, int y, LayerManager &manager) override;

private:
    void calculateBezierPoint(float t, int p0, int p1, int p2, int &out)
    {
        float u = 1.0f - t;
        out = std::round((u * u * p0) + (2.0f * u * t * p1) + (t * t * p2));
    }
};

// ==========================================
// 2. THE SHAPE ARCHETYPE
// ==========================================
class ShapeTool : public Tool
{
protected:
    bool isDrawing = false;
    int startX = 0;
    int startY = 0;

    // Pure virtual functions that child classes MUST implement
    virtual void drawShapePreview(int sx, int sy, int cx, int cy, LayerManager &manager) = 0;
    virtual void drawShapeFinal(int sx, int sy, int cx, int cy, LayerManager &manager) = 0;

public:
    void onPress(int x, int y, LayerManager &manager) override;
    void onMove(int x, int y, LayerManager &manager) override;
    void onRelease(int x, int y, LayerManager &manager) override;
};

// ==========================================
// 3. THE CONCRETE TOOLS
// ==========================================
class BrushTool : public StrokeTool
{
protected:
    void drawLineSegment(int x0, int y0, int x1, int y1, LayerManager &manager) override;
    void drawHoverCursor(int x, int y, LayerManager &manager) override;
};

class EraserTool : public StrokeTool
{
protected:
    void drawLineSegment(int x0, int y0, int x1, int y1, LayerManager &manager) override;
    void drawHoverCursor(int x, int y, LayerManager &manager) override;
};

class RectangleTool : public ShapeTool
{
protected:
    void drawShapePreview(int sx, int sy, int cx, int cy, LayerManager &manager) override;
    void drawShapeFinal(int sx, int sy, int cx, int cy, LayerManager &manager) override;
};

class EllipseTool : public ShapeTool
{
protected:
    void drawShapePreview(int sx, int sy, int cx, int cy, LayerManager &manager) override;
    void drawShapeFinal(int sx, int sy, int cx, int cy, LayerManager &manager) override;
};

class FillTool : public Tool
{
public:
    void onPress(int x, int y, LayerManager &manager) override;

    // We leave Move, Release, and Hover completely blank for the Fill tool
    void onMove(int x, int y, LayerManager &manager) override {}
    void onRelease(int x, int y, LayerManager &manager) override {}
    void onHover(int x, int y, LayerManager &manager) override
    {
        manager.clearPreview(); // Just wipe any previous cursors
    }
};

#endif // TOOL_H