#ifndef LAYER_H
#define LAYER_H

#include <string>
#include <vector>
#include <cstdint>

class Layer
{
public:
    Layer(int width, int height, const std::string &name = "New Layer");

    int width;
    int height;
    std::string name;
    bool visible;
    float opacity; // 0.0f to 1.0f

    // The raw pixel data (4 bytes per pixel: RGBA)
    std::vector<uint8_t> pixels;

    void clear();
    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void fill(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
};

#endif // LAYER_H