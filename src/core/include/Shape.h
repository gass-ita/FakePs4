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
    virtual void draw(LayerManager &manager) const = 0;
};

class Line : public Shape
{
public:
    int x0, y0, x1, y1;
    uint8_t r, g, b, a;
    int thickness;

    Line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255, int thickness = 1);
    void draw(LayerManager &manager) const override;
};

// composite shape
class ShapeGroup : public Shape
{
public:
    std::vector<std::shared_ptr<Shape>> children;

    void addShape(std::shared_ptr<Shape> shape);
    void draw(LayerManager &manager) const override;
};

class Rectangle : public ShapeGroup
{
public:
    Rectangle(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255, int thickness = 1);
};

class Ellipse : public Shape
{
public:
    int xc, yc; // Center coordinates
    int rx, ry; // X-radius and Y-radius
    uint8_t r, g, b, a;
    int thickness;

    Ellipse(int xc, int yc, int rx, int ry, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255, int thickness = 1);
    void draw(LayerManager &manager) const override;
};

#endif // SHAPE_H