#include "Shape.h"
#include <cmath>

// --- Line ---
Line::Line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, uint8_t a, int thickness)
    : x0(x0), y0(y0), x1(x1), y1(y1), r(r), g(g), b(b), a(a), thickness(thickness) {}

void Line::draw(LayerManager &manager) const
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
                        manager.setPixel(cx_center + cx, cy_center + cy, r, g, b, a);
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

void ShapeGroup::draw(LayerManager &manager) const
{
    // A group simply tells all its children to draw themselves!
    for (const auto &child : children)
    {
        child->draw(manager);
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

void Ellipse::draw(LayerManager &manager) const
{
    int radius = thickness / 2;
    int radiusSquared = radius * radius;

    // A reusable helper to stamp thick pixels symmetrically
    auto stampCircle = [&](int cx, int cy)
    {
        if (thickness <= 1)
        {
            manager.setPixel(cx, cy, r, g, b, a);
        }
        else
        {
            for (int cy_off = -radius; cy_off <= radius; cy_off++)
            {
                for (int cx_off = -radius; cx_off <= radius; cx_off++)
                {
                    if (cx_off * cx_off + cy_off * cy_off <= radiusSquared)
                    {
                        manager.setPixel(cx + cx_off, cy + cy_off, r, g, b, a);
                    }
                }
            }
        }
    };

    // Fast Midpoint Ellipse Algorithm (Integer Math)
    long rxSq = rx * rx;
    long rySq = ry * ry;
    long x = 0, y = ry;
    long px = 0, py = 2 * rxSq * y;

    // Region 1 (Top and Bottom curves)
    long p1 = rySq - (rxSq * ry) + (0.25 * rxSq);
    while (px < py)
    {
        stampCircle(xc + x, yc + y);
        stampCircle(xc - x, yc + y);
        stampCircle(xc + x, yc - y);
        stampCircle(xc - x, yc - y);
        x++;
        px += 2 * rySq;
        if (p1 < 0)
        {
            p1 += rySq + px;
        }
        else
        {
            y--;
            py -= 2 * rxSq;
            p1 += rySq + px - py;
        }
    }

    // Region 2 (Left and Right curves)
    long p2 = rySq * (x + 0.5) * (x + 0.5) + rxSq * (y - 1) * (y - 1) - rxSq * rySq;
    while (y >= 0)
    {
        stampCircle(xc + x, yc + y);
        stampCircle(xc - x, yc + y);
        stampCircle(xc + x, yc - y);
        stampCircle(xc - x, yc - y);
        y--;
        py -= 2 * rxSq;
        if (p2 > 0)
        {
            p2 += rxSq - py;
        }
        else
        {
            x++;
            px += 2 * rySq;
            p2 += rxSq - py + px;
        }
    }
}

// Add the implementation:
CircleOutline::CircleOutline(int xc, int yc, int size, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    : xc(xc), yc(yc), size(size), r(r), g(g), b(b), a(a) {}

void CircleOutline::draw(LayerManager &manager) const
{
    int radius = size / 2;
    if (radius == 0)
    { // If size is 1, just draw a dot
        manager.setPixel(xc, yc, r, g, b, a);
        return;
    }

    int x = 0, y = radius;
    int d = 3 - 2 * radius;

    // Helper lambda to draw the 8 symmetric points of a circle outline
    auto drawSymmetric = [&](int cx, int cy)
    {
        manager.setPixel(xc + cx, yc + cy, r, g, b, a);
        manager.setPixel(xc - cx, yc + cy, r, g, b, a);
        manager.setPixel(xc + cx, yc - cy, r, g, b, a);
        manager.setPixel(xc - cx, yc - cy, r, g, b, a);
        manager.setPixel(xc + cy, yc + cx, r, g, b, a);
        manager.setPixel(xc - cy, yc + cx, r, g, b, a);
        manager.setPixel(xc + cy, yc - cx, r, g, b, a);
        manager.setPixel(xc - cy, yc - cx, r, g, b, a);
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