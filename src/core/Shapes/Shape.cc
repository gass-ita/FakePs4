#include "Shape.h"
#include <cmath>

// --- Line ---
Line::Line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, uint8_t a, int thickness)
    : x0(x0), y0(y0), x1(x1), y1(y1), r(r), g(g), b(b), a(a), thickness(thickness) {}

void Line::draw(LayerManager &manager, PixelSetter setter) const
{
    int x_curr = x0;
    int y_curr = y0;

    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    if (thickness <= 1)
    {
        // Standard Bresenham for fast, 1-pixel thin lines
        while (true)
        {
            (manager.*setter)(x_curr, y_curr, r, g, b, a);
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
    else
    {
        // Professional Step-Stamping for thick lines
        int radius = thickness / 2;
        int radiusSquared = radius * radius;

        // 25% spacing for a smooth edge without burning CPU
        float spacing = std::max(1.0f, radius * 0.25f);
        float distSinceLastStamp = spacing; // Start ready to stamp immediately

        // Helper lambda to stamp a solid circle at a given center coordinate
        auto stampCircle = [&](int cx_center, int cy_center)
        {
            for (int cy = -radius; cy <= radius; cy++)
            {
                for (int cx = -radius; cx <= radius; cx++)
                {
                    if (cx * cx + cy * cy <= radiusSquared)
                    {
                        (manager.*setter)(cx_center + cx, cy_center + cy, r, g, b, a);
                    }
                }
            }
        };

        // Main line traversal loop
        while (true)
        {
            if (distSinceLastStamp >= spacing)
            {
                stampCircle(x_curr, y_curr);
                distSinceLastStamp = 0.0f; // Reset counter
            }

            if (x_curr == x1 && y_curr == y1)
                break;

            e2 = 2 * err;
            bool steppedX = false, steppedY = false;

            if (e2 >= dy)
            {
                err += dy;
                x_curr += sx;
                steppedX = true;
            }
            if (e2 <= dx)
            {
                err += dx;
                y_curr += sy;
                steppedY = true;
            }

            // Add actual distance moved (~1.414 for diagonal, 1.0 for straight)
            distSinceLastStamp += (steppedX && steppedY) ? 1.414f : 1.0f;
        }

        // CRITICAL: Always stamp the final cap to ensure the line ends perfectly round
        stampCircle(x1, y1);
    }
}

// --- ShapeGroup ---
void ShapeGroup::addShape(std::shared_ptr<Shape> shape)
{
    children.push_back(shape);
}

void ShapeGroup::draw(LayerManager &manager, PixelSetter setter) const
{
    // A group simply tells all its children to draw themselves!
    for (const auto &child : children)
    {
        child->draw(manager, setter);
    }
}

// --- Rectangle ---
Rectangle::Rectangle(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a, int thickness)
{
    // Compose the rectangle using four lines
    addShape(std::make_shared<Line>(x, y, x + w, y, r, g, b, a, thickness));         // Top edge
    addShape(std::make_shared<Line>(x + w, y, x + w, y + h, r, g, b, a, thickness)); // Right edge
    addShape(std::make_shared<Line>(x + w, y + h, x, y + h, r, g, b, a, thickness)); // Bottom edge
    addShape(std::make_shared<Line>(x, y + h, x, y, r, g, b, a, thickness));         // Left edge
}

// ==========================================
// ELLIPSE
// ==========================================
Ellipse::Ellipse(int xc, int yc, int rx, int ry, uint8_t r, uint8_t g, uint8_t b, uint8_t a, int thickness)
    : xc(xc), yc(yc), rx(rx), ry(ry), r(r), g(g), b(b), a(a), thickness(thickness) {}

void Ellipse::draw(LayerManager &manager, PixelSetter setter) const
{
    float halfThick = thickness / 2.0f;
    float rxOut = rx + halfThick;
    float ryOut = ry + halfThick;
    float rxIn = rx - halfThick;
    float ryIn = ry - halfThick;

    // Prevent inverted inner bounds on very thick small ellipses
    if (rxIn < 0)
        rxIn = 0;
    if (ryIn < 0)
        ryIn = 0;

    float rxOutSq = rxOut * rxOut;
    float ryOutSq = ryOut * ryOut;
    float rxInSq = rxIn * rxIn;
    float ryInSq = ryIn * ryIn;

    // We scanline from the center outwards along the Y axis
    for (int y = 0; y <= std::ceil(ryOut); ++y)
    {
        float ySq = y * y;

        // 1. Find the outer X boundary for this specific Y row
        float valOut = 1.0f - (ySq / ryOutSq);
        int xOut = (valOut >= 0.0f) ? std::round(rxOut * std::sqrt(valOut)) : 0;

        // 2. Find the inner X boundary (if this row is inside the hollow center)
        int xIn = 0;
        if (y <= ryIn && ryIn > 0.0f)
        {
            float valIn = 1.0f - (ySq / ryInSq);
            xIn = (valIn >= 0.0f) ? std::round(rxIn * std::sqrt(valIn)) : 0;
        }

        // 3. Draw horizontal scanlines connecting the inner and outer bounds
        // We do all 4 symmetrical quadrants at the same time
        for (int x = xIn; x <= xOut; ++x)
        {
            (manager.*setter)(xc + x, yc + y, r, g, b, a); // Bottom Right

            if (x > 0)
                (manager.*setter)(xc - x, yc + y, r, g, b, a); // Bottom Left

            if (y > 0)
            {
                (manager.*setter)(xc + x, yc - y, r, g, b, a); // Top Right
                if (x > 0)
                    (manager.*setter)(xc - x, yc - y, r, g, b, a); // Top Left
            }
        }
    }
}

// Add the implementation:
CircleOutline::CircleOutline(int xc, int yc, int size, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    : xc(xc), yc(yc), size(size), r(r), g(g), b(b), a(a) {}

void CircleOutline::draw(LayerManager &manager, PixelSetter setter) const
{
    int radius = size / 2;
    if (radius == 0)
    { // If size is 1, just draw a dot
        (manager.*setter)(xc, yc, r, g, b, a);
        return;
    }

    int x = 0, y = radius;
    int d = 3 - 2 * radius;

    // Helper lambda to draw the 8 symmetric points of a circle outline
    auto drawSymmetric = [&](int cx, int cy)
    {
        (manager.*setter)(xc + cx, yc + cy, r, g, b, a);
        (manager.*setter)(xc - cx, yc + cy, r, g, b, a);
        (manager.*setter)(xc + cx, yc - cy, r, g, b, a);
        (manager.*setter)(xc - cx, yc - cy, r, g, b, a);
        (manager.*setter)(xc + cy, yc + cx, r, g, b, a);
        (manager.*setter)(xc - cy, yc + cx, r, g, b, a);
        (manager.*setter)(xc + cy, yc - cx, r, g, b, a);
        (manager.*setter)(xc - cy, yc - cx, r, g, b, a);
    };

    while (y >= x)
    {
        drawSymmetric(x, y);
        x++;
        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
        {
            d = d + 4 * x + 6;
        }
    }
}