#include "ColorLayer.h"

ColorLayer::ColorLayer(int w, int h, const std::string &n) : TiledLayer(w, h, n, 4) {}

void ColorLayer::setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
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
    int pxIdx = (ly * TILE_SIZE + lx) * 4;

    tiles[tileIdx]->data[pxIdx] = r;
    tiles[tileIdx]->data[pxIdx + 1] = g;
    tiles[tileIdx]->data[pxIdx + 2] = b;
    tiles[tileIdx]->data[pxIdx + 3] = a;
}

void ColorLayer::getPixel(int x, int y, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) const
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
    int pxIdx = (ly * TILE_SIZE + lx) * 4;

    r = tiles[tileIdx]->data[pxIdx];
    g = tiles[tileIdx]->data[pxIdx + 1];
    b = tiles[tileIdx]->data[pxIdx + 2];
    a = tiles[tileIdx]->data[pxIdx + 3];
}

void ColorLayer::fill(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            setPixel(x, y, r, g, b, a);
        }
    }
}