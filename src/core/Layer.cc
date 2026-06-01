#include "Layer.h"
#include <algorithm>
#include <memory>

Layer::Layer(int w, int h, const std::string &n) : width(w), height(h), name(n)
{
    // Ceiling division: A 100x100 canvas needs a 2x2 grid of 64px tiles
    tilesX = (width + TILE_SIZE - 1) / TILE_SIZE;
    tilesY = (height + TILE_SIZE - 1) / TILE_SIZE;

    // Initialize the grid with null pointers (Zero memory used for pixels!)
    tiles.resize(tilesX * tilesY);
}

void Layer::setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (x < 0 || x >= width || y < 0 || y >= height)
        return;

    // 1. Find the Tile Index
    int tx = x / TILE_SIZE;
    int ty = y / TILE_SIZE;
    int tileIdx = ty * tilesX + tx;

    // 2. Lazy Allocation! Only reserve RAM if the brush actually touched here
    if (!tiles[tileIdx])
    {
        tiles[tileIdx] = std::make_unique<Tile>();
    }

    // 3. Find the local coordinates inside the specific 64x64 tile
    int lx = x % TILE_SIZE;
    int ly = y % TILE_SIZE;
    int pxIdx = (ly * TILE_SIZE + lx) * 4;

    tiles[tileIdx]->pixels[pxIdx] = r;
    tiles[tileIdx]->pixels[pxIdx + 1] = g;
    tiles[tileIdx]->pixels[pxIdx + 2] = b;
    tiles[tileIdx]->pixels[pxIdx + 3] = a;
}

void Layer::getPixel(int x, int y, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) const
{
    if (x < 0 || x >= width || y < 0 || y >= height)
    {
        r = g = b = a = 0;
        return;
    }

    int tx = x / TILE_SIZE;
    int ty = y / TILE_SIZE;
    int tileIdx = ty * tilesX + tx;

    // CRITICAL: If the tile pointer is null, the tile is empty.
    // We instantly return 0 (transparent) without allocating any memory!
    if (!tiles[tileIdx])
    {
        r = g = b = a = 0;
        return;
    }

    int lx = x % TILE_SIZE;
    int ly = y % TILE_SIZE;
    int pxIdx = (ly * TILE_SIZE + lx) * 4;

    r = tiles[tileIdx]->pixels[pxIdx];
    g = tiles[tileIdx]->pixels[pxIdx + 1];
    b = tiles[tileIdx]->pixels[pxIdx + 2];
    a = tiles[tileIdx]->pixels[pxIdx + 3];
}

void Layer::fill(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    // Warning: Filling forces ALL tiles to be allocated in RAM
    // because every single pixel now has color data!
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            setPixel(x, y, r, g, b, a);
        }
    }
}

void Layer::clear()
{
    // Instead of wiping memory, we can just drop all the tiles!
    for (auto &tile : tiles)
    {
        tile.reset();
    }
}