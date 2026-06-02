#include "TiledLayer.h"

TiledLayer::TiledLayer(int w, int h, const std::string &n, size_t chans)
    : Layer(w, h, n), channels(chans)
{
    tilesX = (width + TILE_SIZE - 1) / TILE_SIZE;
    tilesY = (height + TILE_SIZE - 1) / TILE_SIZE;
    tiles.resize(tilesX * tilesY);
}

void TiledLayer::clear()
{
    for (auto &tile : tiles)
        tile.reset();
}