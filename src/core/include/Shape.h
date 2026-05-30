#ifndef SHAPE_H
#define SHAPE_H

#include "LayerManager.h"
#include <memory>
#include <vector>
#include <cstdint>

// 1. The Component Interface
class Shape
{
public:
    virtual ~Shape() = default;

    // Every shape must know how to draw itself onto the manager
    virtual void draw(LayerManager &manager) const = 0;
};

// 2. The Leaf (Primitive)
class Line : public Shape
{
public:
    int x0, y0, x1, y1;
    uint8_t r, g, b, a;

    Line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void draw(LayerManager &manager) const override;
};

// 3. The Composite
class ShapeGroup : public Shape
{
public:
    std::vector<std::shared_ptr<Shape>> children;

    void addShape(std::shared_ptr<Shape> shape);
    void draw(LayerManager &manager) const override;
};

// 4. Proving the Pattern: A Rectangle composed of 4 lines!
class Rectangle : public ShapeGroup
{
public:
    Rectangle(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
};

#endif // SHAPE_H