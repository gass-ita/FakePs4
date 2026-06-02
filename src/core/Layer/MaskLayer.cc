#include "MaskLayer.h"

MaskLayer::MaskLayer(int w, int h, const std::string &n) : TiledLayer(w, h, n, 1) {}

void MaskLayer::setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (x < 0 || x >= width || y < 0 || y >= height)
        return;

    int tx = x / TILE_SIZE;
    int ty = y / TILE_SIZE;
    int tileIdx = ty * tilesX + tx;

    if (!tiles[tileIdx])
    {
        tiles[tileIdx] = createTile();
    }

    int lx = x % TILE_SIZE;
    int ly = y % TILE_SIZE;
    int pxIdx = (ly * TILE_SIZE + lx);

    tiles[tileIdx]->data[pxIdx] = a; // Only store alpha
}

void MaskLayer::getPixel(int x, int y, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) const
{
    if (x < 0 || x >= width || y < 0 || y >= height)
    {
        r = g = b = a = 0;
        return;
    }

    int tx = x / TILE_SIZE;
    int ty = y / TILE_SIZE;
    int tileIdx = ty * tilesX + tx;

    if (!tiles[tileIdx])
    {
        r = g = b = a = 0;
        return;
    }

    int lx = x % TILE_SIZE;
    int ly = y % TILE_SIZE;
    int pxIdx = (ly * TILE_SIZE + lx);

    a = tiles[tileIdx]->data[pxIdx];
    r = g = b = 0; // Mask layer only has alpha
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