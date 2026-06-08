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
        // ==========================================
        // PRO SOFTWARE RENDERING (Zero Overdraw)
        // ==========================================
        int radius = thickness / 2;
        float radiusSquared = static_cast<float>(radius * radius);

        // 1. Calculate the bounding box of the entire capsule shape
        int minX = std::min(x0, x1) - radius;
        int maxX = std::max(x0, x1) + radius;
        int minY = std::min(y0, y1) - radius;
        int maxY = std::max(y0, y1) + radius;

        // Pre-calculate line segment vector lengths for math speed
        // Pre-calculate line segment vector lengths
        float dx_line = static_cast<float>(x1 - x0);
        float dy_line = static_cast<float>(y1 - y0);
        float lineLengthSquared = (dx_line * dx_line) + (dy_line * dy_line);

        // 1. PRE-CALCULATE INVERSE LENGTH to avoid division inside the loop
        float invLengthSq = (lineLengthSquared > 0.0f) ? (1.0f / lineLengthSquared) : 0.0f;

        // 2. Iterate through the bounding box
        for (int y = minY; y <= maxY; ++y)
        {
            // HOIST CONSTANT Y-MATH: Calculate this once per row, not per pixel
            float dy_base = static_cast<float>(y - y0);
            float y_term = dy_base * dy_line;

            for (int x = minX; x <= maxX; ++x)
            {
                float dx_base = static_cast<float>(x - x0);
                float distSquared;

                if (lineLengthSquared == 0.0f)
                {
                    // Single dot (clicked without dragging)
                    distSquared = (dx_base * dx_base) + (dy_base * dy_base);
                }
                else
                {
                    // 3. MULTIPLY instead of divide for extreme speed
                    float t = (dx_base * dx_line + y_term) * invLengthSq;

                    // 4. Fast clamp (slightly faster than std::max/min)
                    t = (t < 0.0f) ? 0.0f : ((t > 1.0f) ? 1.0f : t);

                    float closestX = x0 + t * dx_line;
                    float closestY = y0 + t * dy_line;

                    float dx_pt = static_cast<float>(x) - closestX;
                    float dy_pt = static_cast<float>(y) - closestY;
                    distSquared = (dx_pt * dx_pt) + (dy_pt * dy_pt);
                }

                // 5. If within radius, draw it
                if (distSquared <= radiusSquared)
                {
                    (manager.*setter)(x, y, r, g, b, a);
                }
            }
        }
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

SprayShape::SprayShape(int xc, int yc, uint8_t r, uint8_t g, uint8_t b, uint8_t a, int size, float density) : xc(xc), yc(yc), r(r), g(g), b(b), a(a), size(size), density(density) {}

void SprayShape::draw(LayerManager &manager, PixelSetter setter) const
{
    // generate 2 random numbers a radius and an angle
    for (int i = 0; i < size * density; ++i) // number of dots is proportional to size
    {
        // To produce a uniform distribution over the circle area,
        // sample radius = sqrt(u) * size where u ~ U(0,1).
        float u = static_cast<float>(rand()) / RAND_MAX;
        float radius = std::sqrt(u) * size;
        float angle = static_cast<float>(rand()) / RAND_MAX * 6.2831853f; // 0 to 2PI

        int px = xc + static_cast<int>(radius * std::cos(angle));
        int py = yc + static_cast<int>(radius * std::sin(angle));

        (manager.*setter)(px, py, r, g, b, a);
    }
}
