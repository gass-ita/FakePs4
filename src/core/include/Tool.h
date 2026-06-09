#ifndef TOOL_H
#define TOOL_H
#include "LayerManager.h"
#include "Shape.h" // Assuming this is where Line, Rectangle, Ellipse are
#include <deque>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <memory>

class Tool
{
protected:
    virtual bool requiresHighFrequency() const { return true; }
    short int throttleMs = 16;        // lowfreq tools
    short int highFreqThrottleMs = 5; // highfreq tools (like the brush) can skip throttling altogether for maximum responsiveness
    std::chrono::steady_clock::time_point lastTime;

public:
    int size = 5;
    uint8_t r = 0, g = 0, b = 0, a = 255;

    Tool() : lastTime(std::chrono::steady_clock::now()) {}

    // input throttling logic to prevent tools from overwhelming the CPU with too many updates (especially important for the Shape tool)
    bool shouldProcessMove()
    {

        short _throttleMs = requiresHighFrequency() ? highFreqThrottleMs : throttleMs;
        if (_throttleMs > 0)
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();

            if (elapsed < _throttleMs)
                return false;

            lastTime = now;
        }

        // Update tracking and allow the move!
        return true;
    }

    virtual ~Tool() = default;

    virtual void setSize(int newSize) { size = newSize; }
    virtual void setColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
    {
        r = red;
        g = green;
        b = blue;
        a = alpha;
    }

    virtual void onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) {}
    virtual void onMove(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) {}
    virtual void onRelease(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) {}
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

    int sumX = 0;
    int sumY = 0;

    short int smoothingWindow = 6; // the higher the smoother but more delay
    std::deque<std::pair<int, int>> pointHistory;

    // Pure virtual functions that child classes MUST implement
    virtual void drawLineSegment(int x0, int y0, int x1, int y1, LayerManager &manager) = 0;
    virtual void drawHoverCursor(int x, int y, LayerManager &manager) = 0;

public:
    void onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override;
    void onMove(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override;
    void onRelease(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override;
    void onHover(int x, int y, LayerManager &manager) override;

private:
    void calculateBezierPoint(float t, int p0, int p1, int p2, int &out)
    {
        float u = 1.0f - t;
        out = std::round((u * u * p0) + (2.0f * u * t * p1) + (t * t * p2));
    }
};

// ...

class MaskBrushTool : public StrokeTool
{
private:
    std::shared_ptr<MaskLayer> brushTip;

    float distanceAccumulator = 0.0f;

    // The internal function that physically applies the mask to the canvas
    void stampMask(int cx, int cy, LayerManager &manager);

public:
    // Pass the texture in when you create the tool!
    MaskBrushTool(std::shared_ptr<MaskLayer> mask);
    void onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override;

#include <memory>
#include <cmath>

    // diameter: The total pixel size of the mask texture
    // hardness: 0.0f (Airbrush) to 1.0f (Hard Pen)
    static std::shared_ptr<MaskLayer> createRoundBrushMask(int diameter, float hardness = 0.8f)
    {
        // Ensure we don't try to create a 0-pixel mask
        if (diameter < 1)
            diameter = 1;

        auto mask = std::make_shared<MaskLayer>(diameter, diameter, "BrushTip");

        // The exact mathematical center of the texture
        float cx = diameter / 2.0f;
        float cy = diameter / 2.0f;
        float radius = diameter / 2.0f;

        // Calculate where the solid center ends, and the soft fade begins
        float fadeStartRadius = radius * hardness;

        // THE ANTI-ALIASING FIX:
        // Even on a 100% hard brush, we MUST force at least 1 pixel of fade at the edge
        // so the pixels seamlessly blend into the background instead of looking jagged.
        if (radius - fadeStartRadius < 1.0f)
        {
            fadeStartRadius = std::max(0.0f, radius - 1.0f);
        }

        for (int y = 0; y < diameter; ++y)
        {
            for (int x = 0; x < diameter; ++x)
            {

                // Pro-Tip: Add 0.5 to measure from the absolute center of the pixel,
                // preventing the circle from looking lopsided at very small sizes!
                float px = x + 0.5f;
                float py = y + 0.5f;

                // Calculate distance from center using the Pythagorean theorem
                float dist = std::sqrt(std::pow(px - cx, 2) + std::pow(py - cy, 2));

                uint8_t alpha = 0;

                if (dist <= fadeStartRadius)
                {
                    // 1. Inside the solid core
                    alpha = 255;
                }
                else if (dist < radius)
                {
                    // 2. Inside the anti-aliased edge!
                    // Calculate exactly how far along the gradient we are (0.0 to 1.0)
                    float fadeLength = radius - fadeStartRadius;
                    float distIntoFade = dist - fadeStartRadius;

                    // Invert it so it goes from 1.0 (inside) to 0.0 (outside edge)
                    float falloff = 1.0f - (distIntoFade / fadeLength);

                    // Multiply by 255 to get the final alpha
                    alpha = static_cast<uint8_t>(falloff * 255.0f);
                }

                // Write the pixel to the mask (Color parameters are 0 because they are ignored)
                if (alpha > 0)
                {
                    mask->setPixel(x, y, 0, 0, 0, alpha);
                }
            }
        }

        return mask;
    }

protected:
    void drawLineSegment(int x0, int y0, int x1, int y1, LayerManager &manager) override;
    void drawHoverCursor(int x, int y, LayerManager &manager) override;
};

class ColorPickerTool : public Tool
{
public:
    void onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override;
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
    int lastX = -1;
    int lastY = -1;

    // Pure virtual functions that child classes MUST implement
    virtual void drawShapePreview(int sx, int sy, int cx, int cy, LayerManager &manager) = 0;
    virtual void drawShapeFinal(int sx, int sy, int cx, int cy, LayerManager &manager) = 0;
    virtual bool requiresHighFrequency() const override { return false; } // Shapes don't need high-frequency updates
public:
    void onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override;
    void onMove(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override;
    void onRelease(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override;
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

// ==========================================
// DECORATOR BASE CLASS
// ==========================================
class ToolDecorator : public Tool
{
protected:
    std::unique_ptr<Tool> wrappedTool;

public:
    ToolDecorator(std::unique_ptr<Tool> tool) : wrappedTool(std::move(tool))
    {
        // Sync the base variables when created
        if (this->wrappedTool)
        {
            this->size = this->wrappedTool->size;
            this->r = this->wrappedTool->r;
            this->g = this->wrappedTool->g;
            this->b = this->wrappedTool->b;
            this->a = this->wrappedTool->a;
        }
    }

    // Intercept and pass down UI settings
    void setSize(int newSize) override
    {
        Tool::setSize(newSize);
        if (wrappedTool)
            wrappedTool->setSize(newSize);
    }
    void setColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) override
    {
        Tool::setColor(red, green, blue, alpha);
        if (wrappedTool)
            wrappedTool->setColor(red, green, blue, alpha);
    }

    // Intercept and pass down canvas events
    void onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override
    {
        if (wrappedTool)
            wrappedTool->onPress(x, y, pressure, tiltX, tiltY, manager);
    }
    void onMove(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override
    {
        if (wrappedTool)
            wrappedTool->onMove(x, y, pressure, tiltX, tiltY, manager);
    }
    void onRelease(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override
    {
        if (wrappedTool)
            wrappedTool->onRelease(x, y, pressure, tiltX, tiltY, manager);
    }
    void onHover(int x, int y, LayerManager &manager) override
    {
        if (wrappedTool)
            wrappedTool->onHover(x, y, manager);
    }
};

// ==========================================
// CONCRETE DECORATORS
// ==========================================
class PressureSizeDecorator : public ToolDecorator
{
private:
    int originalBaseSize;

public:
    PressureSizeDecorator(std::unique_ptr<Tool> tool);
    void onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override;
    void onMove(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override;
    void onRelease(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override;
};

class ConeGrowthDecorator : public ToolDecorator
{
private:
    float currentDynamicSize = 1.0f;
    float growthRate;
    float growthFactor;
    float calculatedMaxSize = 0.0f;
    int originalBaseSize;
    int rawLastX = 0;
    int rawLastY = 0;

public:
    ConeGrowthDecorator(std::unique_ptr<Tool> tool, float rate = 0.05f, float factor = 1.0f);
    void onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override;
    void onMove(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override;
    void onRelease(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override;
};

class EraserTool : public StrokeTool
{
protected:
    void drawLineSegment(int x0, int y0, int x1, int y1, LayerManager &manager) override;
    void drawHoverCursor(int x, int y, LayerManager &manager) override;
};

class SprayTool : public StrokeTool
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
    void onPress(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override;

    // We leave Move, Release, and Hover completely blank for the Fill tool
    void onMove(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override {}
    void onRelease(int x, int y, float pressure, float tiltX, float tiltY, LayerManager &manager) override {}
    void onHover(int x, int y, LayerManager &manager) override
    {
        manager.clearPreview(); // Just wipe any previous cursors
    }
};

#endif // TOOL_H