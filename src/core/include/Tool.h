#ifndef TOOL_H
#define TOOL_H

#include "LayerManager.h"
#include "Shape.h"

// The Strategy Interface
class Tool
{
protected:
    int size = 5;
    uint8_t r = 0, g = 0, b = 0, a = 255; // active color (default black)

public:
    virtual ~Tool() = default;

    virtual void setSize(int newSize) { size = newSize; }
    // int getSize() const { return size; }
    virtual void setColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
    {
        r = red;
        g = green;
        b = blue;
        a = alpha;
    }

    // Every tool must know how to handle these three mouse states
    virtual void onPress(int x, int y, LayerManager &manager) = 0;
    virtual void onMove(int x, int y, LayerManager &manager) = 0;
    virtual void onRelease(int x, int y, LayerManager &manager) = 0;
    virtual void onHover(int /*x*/, int /*y*/, LayerManager & /*manager*/) {}
};

class BrushTool : public Tool
{
public:
    void onPress(int x, int y, LayerManager &manager) override;
    void onMove(int x, int y, LayerManager &manager) override;
    void onRelease(int x, int y, LayerManager &manager) override;
    void onHover(int x, int y, LayerManager &manager) override;

private:
    int lastX = 0, lastY = 0;
    bool isDrawing = false;
};

class RectangleTool : public Tool
{
public:
    void onPress(int x, int y, LayerManager &manager) override;
    void onMove(int x, int y, LayerManager &manager) override;
    void onRelease(int x, int y, LayerManager &manager) override;

private:
    int startX = 0, startY = 0;
    bool isDrawing = false;
};

// Tool 4: The Ellipse Stamp
class EllipseTool : public Tool
{
public:
    void onPress(int x, int y, LayerManager &manager) override;
    void onMove(int x, int y, LayerManager &manager) override;
    void onRelease(int x, int y, LayerManager &manager) override;

private:
    int startX = 0, startY = 0;
    bool isDrawing = false;
};

// Tool 3: The Eraser
class EraserTool : public Tool
{
public:
    void onPress(int x, int y, LayerManager &manager) override;
    void onMove(int x, int y, LayerManager &manager) override;
    void onRelease(int x, int y, LayerManager &manager) override;
    void onHover(int x, int y, LayerManager &manager) override;

private:
    int lastX = 0, lastY = 0;
    bool isErasing = false;
};

#endif // TOOL_H