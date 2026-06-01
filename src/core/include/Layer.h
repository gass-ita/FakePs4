#ifndef LAYER_H
#define LAYER_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

constexpr int TILE_SIZE = 64;

struct Tile
{
    std::vector<uint8_t> pixels;
    Tile() : pixels(TILE_SIZE * TILE_SIZE * 4, 0) {}
};

class Layer
{
public:
    Layer(int width, int height, const std::string &name = "New Layer");

    int width;
    int height;
    std::string name;
    bool visible = true;
    float opacity = 1.0f; // 0.0f to 1.0f

    int tilesX;
    int tilesY;

    std::vector<std::unique_ptr<Tile>> tiles;

    void clear();
    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void getPixel(int x, int y, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) const;
    void fill(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
};

#endif // LAYER_H