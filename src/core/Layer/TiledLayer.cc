#include "TiledLayer.h"

TiledLayer::TiledLayer(int w, int h, const std::string &n, size_t chans)
    : Layer(w, h, n), channels(chans)
{
    tilesX = (width + TILE_SIZE - 1) / TILE_SIZE;
    tilesY = (height + TILE_SIZE - 1) / TILE_SIZE;
    tiles.resize(tilesX * tilesY);
}

const Tile *TiledLayer::getTile(int tileX, int tileY) const
{
    // 1. Safety bounds check (crucial)
    if (tileX < 0 || tileX >= tilesX || tileY < 0 || tileY >= tilesY)
    {
        return nullptr;
    }

    // 2. Calculate the flat index
    int index = tileY * tilesX + tileX;

    // 3. Use .get() to extract the raw pointer from the unique_ptr!
    return tiles[index].get();
}

void TiledLayer::clear()
{
    for (auto &tile : tiles)
        tile.reset();
}