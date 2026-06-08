#ifndef TILEDLAYER_H
#define TILEDLAYER_H

#include "Layer.h"
#include <vector>
#include <memory>

constexpr int TILE_BITS = 8;              // Because 2^8 = 256
constexpr int TILE_SIZE = 1 << TILE_BITS; // 2^8 = 256

// Abstract Tile storage
struct Tile
{
    std::vector<uint8_t> data;
    Tile(size_t size) : data(size, 0) {}
};

class TiledLayer : public Layer
{
protected:
    int tilesX, tilesY;
    std::vector<std::unique_ptr<Tile>> tiles;
    size_t channels;

    std::unique_ptr<Tile> createTile() const
    {
        return std::make_unique<Tile>(TILE_SIZE * TILE_SIZE * channels);
    }

public:
    TiledLayer(int w, int h, const std::string &n, size_t chans);

    const Tile *getTile(int tileX, int tileY) const;
    void clear() override;
    void fill(uint8_t r, uint8_t g, uint8_t b, uint8_t a) override = 0;
};

#endif // TILEDLAYER_H