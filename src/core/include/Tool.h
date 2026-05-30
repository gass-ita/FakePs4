#ifndef TOOL_H
#define TOOL_H

#include "LayerManager.h"
#include "Shape.h"

// The Strategy Interface
class Tool
{
public:
    virtual ~Tool() = default;

    // Every tool must know how to handle these three mouse states
    virtual void onPress(int x, int y, LayerManager &manager) = 0;
    virtual void onMove(int x, int y, LayerManager &manager) = 0;
    virtual void onRelease(int x, int y, LayerManager &manager) = 0;
};

class BrushTool : public Tool
{
public:
    void onPress(int x, int y, LayerManager &manager) override;
    void onMove(int x, int y, LayerManager &manager) override;
    void onRelease(int x, int y, LayerManager &manager) override;

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
};

// Tool 3: The Eraser
class EraserTool : public Tool
{
public:
    void onPress(int x, int y, LayerManager &manager) override;
    void onMove(int x, int y, LayerManager &manager) override;
    void onRelease(int x, int y, LayerManager &manager) override;

private:
    int lastX = 0, lastY = 0;
    bool isErasing = false;
};

#endif // TOOL_H