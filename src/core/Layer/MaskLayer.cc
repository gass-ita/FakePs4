#include "MaskLayer.h"

MaskLayer::MaskLayer(int w, int h, const std::string &n) : TiledLayer(w, h, n, 1) {}

void MaskLayer::setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (x < 0 || x >= width || y < 0 || y >= height)
        return;

    int tx = x >> TILE_BITS; // was: x / TILE_SIZE
    int ty = y >> TILE_BITS; // was: y / TILE_SIZE
    int tileIdx = ty * tilesX + tx;

    if (!tiles[tileIdx])
        tiles[tileIdx] = createTile();

    int lx = x & (TILE_SIZE - 1);       // was: x % TILE_SIZE
    int ly = y & (TILE_SIZE - 1);       // was: y % TILE_SIZE
    int pxIdx = (ly << TILE_BITS) + lx; // was: ly * TILE_SIZE + lx

    tiles[tileIdx]->data[pxIdx] = a;
}

void MaskLayer::getPixel(int x, int y, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) const
{
    if (x < 0 || x >= width || y < 0 || y >= height)
    {
        r = g = b = a = 0;
        return;
    }

    int tx = x >> TILE_BITS;
    int ty = y >> TILE_BITS;
    int tileIdx = ty * tilesX + tx;

    if (!tiles[tileIdx])
    {
        r = g = b = a = 0;
        return;
    }

    int lx = x & (TILE_SIZE - 1);
    int ly = y & (TILE_SIZE - 1);
    int pxIdx = (ly << TILE_BITS) + lx;

    a = tiles[tileIdx]->data[pxIdx];
    r = g = b = 0;
}

void MaskLayer::fill(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            setPixel(x, y, 0, 0, 0, a); // Only set alpha
        }
    }
}