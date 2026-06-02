#ifndef LAYER_H
#define LAYER_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

#include <string>
#include <cstdint>

class Layer
{
public:
    enum class Type : uint8_t
    {
        Color = 0,
        Mask = 1
    };

public:
    Layer(int w, int h, const std::string &n) : width(w), height(h), name(n) {}
    virtual ~Layer() = default;

    int width, height;
    std::string name;
    bool visible = true;
    float opacity = 1.0f;

    // Interface methods that every layer must implement
    virtual void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) = 0;
    virtual void getPixel(int x, int y, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) const = 0;
    virtual void clear() = 0;
    virtual void fill(uint8_t r, uint8_t g, uint8_t b, uint8_t a) = 0;
    virtual Layer::Type getType() const = 0;
};

#endif // LAYER_H