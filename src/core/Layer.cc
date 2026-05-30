#include "Layer.h"
#include <algorithm>

Layer::Layer(int width, int height, const std::string &name)
    : width(width), height(height), name(name), visible(true), opacity(1.0f)
{
    // Resize vector to hold all pixels (width * height * 4 channels)
    // and initialize to 0 (completely transparent)
    pixels.resize(width * height * 4, 0);
}

void Layer::clear()
{
    std::fill(pixels.begin(), pixels.end(), 0);
}

void Layer::setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (x < 0 || x >= width || y < 0 || y >= height)
        return;

    // Calculate 1D index from 2D coordinates
    int index = (y * width + x) * 4;

    pixels[index] = r;
    pixels[index + 1] = g;
    pixels[index + 2] = b;
    pixels[index + 3] = a;
}