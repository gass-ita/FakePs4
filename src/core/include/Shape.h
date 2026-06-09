#ifndef SHAPE_H
#define SHAPE_H

#include "LayerManager.h"
#include <memory>
#include <vector>
#include <cstdint>

using PixelSetter = void (LayerManager::*)(int, int, uint8_t, uint8_t, uint8_t, uint8_t);

// 1. The Component Interface
class Shape
{
public:
    virtual ~Shape() = default;
    virtual void draw(LayerManager &manager, PixelSetter setter = &LayerManager::setPixel) const = 0;
};

class Line : public Shape
{
public:
    int x0, y0, x1, y1;
    uint8_t r, g, b, a;
    int thickness;

    Line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255, int thickness = 1);
    void draw(LayerManager &manager, PixelSetter setter = &LayerManager::setPixel) const override;
};

class SprayShape : public Shape
{
private:
    int xc, yc;
    uint8_t r, g, b, a;
    int size;
    float density;

public:
    SprayShape(int xc, int yc, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255, int size = 10, float density = 1.0f);
    virtual void draw(LayerManager &manager, PixelSetter setter = &LayerManager::setPixel) const override;
};

// composite shape
class ShapeGroup : public Shape
{
public:
    std::vector<std::shared_ptr<Shape>> children;

    void addShape(std::shared_ptr<Shape> shape);
    void draw(LayerManager &manager, PixelSetter setter = &LayerManager::setPixel) const override;
};

// Shape.h — replace the Rectangle class declaration
class Rectangle : public Shape
{
public:
    int x, y, w, h;
    uint8_t r, g, b, a;
    int thickness;

    Rectangle(int x, int y, int w, int h,
              uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255,
              int thickness = 1);

    void draw(LayerManager &manager,
              PixelSetter setter = &LayerManager::setPixel) const override;
};

class Ellipse : public Shape
{
public:
    int xc, yc; // Center coordinates
    int rx, ry; // X-radius and Y-radius
    uint8_t r, g, b, a;
    int thickness;

    Ellipse(int xc, int yc, int rx, int ry, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255, int thickness = 1);
    void draw(LayerManager &manager, PixelSetter setter = &LayerManager::setPixel) const override;
};

class CircleOutline : public Shape
{
public:
    int xc, yc, size;
    uint8_t r, g, b, a;

    CircleOutline(int xc, int yc, int size, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void draw(LayerManager &manager, PixelSetter setter = &LayerManager::setPixel) const override;
};

#endif // SHAPE_H