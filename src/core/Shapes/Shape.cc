#include "Shape.h"
#include <cmath>

// --- Line ---
Line::Line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    : x0(x0), y0(y0), x1(x1), y1(y1), r(r), g(g), b(b), a(a) {}

void Line::draw(LayerManager &manager) const
{
    int x_curr = x0;
    int y_curr = y0;

    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (true)
    {
        // Feed the calculated pixel to the manager
        manager.setPixel(x_curr, y_curr, r, g, b, a);

        if (x_curr == x1 && y_curr == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x_curr += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y_curr += sy;
        }
    }
}

// --- ShapeGroup ---
void ShapeGroup::addShape(std::shared_ptr<Shape> shape)
{
    children.push_back(shape);
}

void ShapeGroup::draw(LayerManager &manager) const
{
    // A group simply tells all its children to draw themselves!
    for (const auto &child : children)
    {
        child->draw(manager);
    }
}

// --- Rectangle ---
Rectangle::Rectangle(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    // Compose the rectangle using four lines
    addShape(std::make_shared<Line>(x, y, x + w, y, r, g, b, a));         // Top edge
    addShape(std::make_shared<Line>(x + w, y, x + w, y + h, r, g, b, a)); // Right edge
    addShape(std::make_shared<Line>(x + w, y + h, x, y + h, r, g, b, a)); // Bottom edge
    addShape(std::make_shared<Line>(x, y + h, x, y, r, g, b, a));         // Left edge
}