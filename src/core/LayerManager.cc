#include "LayerManager.h"
#include <algorithm>
#include <iostream>

LayerManager::LayerManager(int width, int height)
    : width(width), height(height)
{
    previewLayer = std::make_shared<Layer>(width, height, "Preview Layer");
    projectionCache.resize(width * height * 4, 0);
    addLayer("Background");

    tilesX = (width + TILE_SIZE - 1) / TILE_SIZE;
    tilesY = (height + TILE_SIZE - 1) / TILE_SIZE;

    layers.back()->fill(255, 255, 255, 255);
    markRegionDirty(0, 0, width, height);
}

void LayerManager::addLayer(const std::string &name)
{
    layers.push_back(std::make_shared<Layer>(width, height, name));
}

void LayerManager::addObserver(LMObserver *observer)
{
    observers.push_back(observer);
}

void LayerManager::markRegionDirty(int x, int y, int w, int h, bool skipCache)
{
    if (!skipCache)
        updateCache(x, y, w, h);

    for (auto *observer : observers)
    {
        observer->onRegionChanged(x, y, w, h);
    }
}

void LayerManager::setActiveLayer(size_t index)
{
    if (index < layers.size())
    {
        activeLayerIndex = index;
    }
}

void LayerManager::setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{

    if (activeLayerIndex >= layers.size())
        return;
    layers[activeLayerIndex]->setPixel(x, y, r, g, b, a);

    if (!isBatching)
    {
        // Normal immediate update
        markRegionDirty(x, y, 1, 1);
    }
}

void LayerManager::getPixel(int x, int y, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) const
{
    if (activeLayerIndex >= layers.size())
    {
        r = g = b = a = 0;
        return;
    }
    layers[activeLayerIndex]->getPixel(x, y, r, g, b, a);
}

void LayerManager::updateCache(int startX, int startY, int rWidth, int rHeight)
{
    int x0 = std::max(0, startX);
    int y0 = std::max(0, startY);
    int x1 = std::min(width, startX + rWidth);
    int y1 = std::min(height, startY + rHeight);

    for (int y = y0; y < y1; ++y)
    {
        for (int x = x0; x < x1; ++x)
        {
            int globalIdx = (y * width + x) * 4;

            // 1. Reset cache pixel to completely transparent
            projectionCache[globalIdx] = 0;
            projectionCache[globalIdx + 1] = 0;
            projectionCache[globalIdx + 2] = 0;
            projectionCache[globalIdx + 3] = 0;

            // 2. Re-composite all layers
            for (const auto &layer : layers)
            {
                if (!layer->visible)
                    continue;

                uint8_t &br = projectionCache[globalIdx];
                uint8_t &bg = projectionCache[globalIdx + 1];
                uint8_t &bb = projectionCache[globalIdx + 2];
                uint8_t &ba = projectionCache[globalIdx + 3];

                uint8_t tr, tg, tb, ta;

                // NEW: Safely ask the layer for the pixel data!
                // If the tile is empty, this instantly returns 0 without allocating memory.
                layer->getPixel(x, y, tr, tg, tb, ta);

                if (ta > 0)
                {
                    blendPixels(br, bg, bb, ba, tr, tg, tb, ta, layer->opacity);
                }
            }
        }
    }
}

std::vector<uint8_t> LayerManager::renderRegion(int startX, int startY, int rWidth, int rHeight) const
{
    int x0 = std::max(0, startX);
    int y0 = std::max(0, startY);
    int x1 = std::min(width, startX + rWidth);
    int y1 = std::min(height, startY + rHeight);

    int w = x1 - x0;
    int h = y1 - y0;

    if (w <= 0 || h <= 0)
        return {};

    std::vector<uint8_t> regionBuffer(w * h * 4, 0);

    // Copy the memory row by row
    for (int y = 0; y < h; ++y)
    {
        int globalIdx = ((y0 + y) * width + x0) * 4;
        int localIdx = (y * w) * 4;

        // std::copy is highly optimized by the compiler for flat memory operations
        std::copy(projectionCache.begin() + globalIdx,
                  projectionCache.begin() + globalIdx + (w * 4),
                  regionBuffer.begin() + localIdx);
    }

    // add the preview layer on top of the cache if it has any pixels in this region
    // add the preview layer on top of the cache if it has any pixels in this region
    if (previewLayer)
    {
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                int localIdx = (y * w + x) * 4;
                int globalX = x0 + x;
                int globalY = y0 + y;

                uint8_t tr, tg, tb, ta;
                // NEW: Use getPixel for the preview layer too
                previewLayer->getPixel(globalX, globalY, tr, tg, tb, ta);

                // Only do math if the preview pixel exists
                if (ta > 0)
                {
                    uint8_t &br = regionBuffer[localIdx];
                    uint8_t &bg = regionBuffer[localIdx + 1];
                    uint8_t &bb = regionBuffer[localIdx + 2];
                    uint8_t &ba = regionBuffer[localIdx + 3];

                    blendPixels(br, bg, bb, ba, tr, tg, tb, ta, 1.0f);
                }
            }
        }
    }

    return regionBuffer;
}

std::vector<uint8_t> LayerManager::render() const
{
    return renderRegion(0, 0, width, height);
}

void LayerManager::blendPixels(uint8_t &outR, uint8_t &outG, uint8_t &outB, uint8_t &outA,
                               uint8_t topR, uint8_t topG, uint8_t topB, uint8_t topA, float opacity) const
{
    int effectiveAlpha = static_cast<int>(topA * opacity);

    if (effectiveAlpha == 0)
        return;

    if (effectiveAlpha == 255)
    {
        outR = topR;
        outG = topG;
        outB = topB;
        outA = 255;
        return;
    }

    int invAlpha = 255 - effectiveAlpha;

    outR = (topR * effectiveAlpha + outR * invAlpha) / 255;
    outG = (topG * effectiveAlpha + outG * invAlpha) / 255;
    outB = (topB * effectiveAlpha + outB * invAlpha) / 255;

    outA = outA + effectiveAlpha - (outA * effectiveAlpha) / 255;
}

// Batching methods
void LayerManager::beginBatch()
{
    isBatching = true;
    dirtyTiles.clear(); // Empty the Set for the new stroke
}

void LayerManager::endBatch()
{
    isBatching = false;

    // Process every unique 64x64 tile that was touched
    for (int tileIndex : dirtyTiles)
    {

        // Translate the ID back into an X/Y grid coordinate
        int tx = tileIndex % tilesX;
        int ty = tileIndex / tilesX;
        // Tell the engine to re-composite ONLY this specific 64x64 square
        markRegionDirty(tx * TILE_SIZE, ty * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    }

    dirtyTiles.clear();
}

void LayerManager::addDirtyRect(int x, int y, int w, int h)
{
    if (!isBatching)
    {
        markRegionDirty(x, y, w, h);
        return;
    }

    // 1. Find the bounds of the rectangle on the 64x64 grid
    // We use std::max and std::min to ensure we never go off the canvas!
    int txStart = std::max(0, x / TILE_SIZE);
    int tyStart = std::max(0, y / TILE_SIZE);
    int txEnd = std::min(tilesX - 1, (x + w) / TILE_SIZE);
    int tyEnd = std::min(tilesY - 1, (y + h) / TILE_SIZE);

    // 2. Loop through the grid and add every touched Tile ID to the Set
    for (int ty = tyStart; ty <= tyEnd; ++ty)
    {
        for (int tx = txStart; tx <= txEnd; ++tx)
        {
            int tileIndex = ty * tilesX + tx;

            // The std::unordered_set automatically ignores duplicates!
            dirtyTiles.insert(tileIndex);
        }
    }
}

void LayerManager::setLayerVisibility(size_t index, bool visible)
{
    if (index >= layers.size())
        return;

    // Only recalculate if the visibility actually changed
    if (layers[index]->visible != visible)
    {
        layers[index]->visible = visible;

        markRegionDirty(0, 0, getWidth(), getHeight());
    }
}

void LayerManager::setLayerName(size_t index, const std::string &name)
{
    if (index >= layers.size())
        return;
    layers[index]->name = name;
}

void LayerManager::setLayerOpacity(size_t index, float opacity)
{
    if (index >= layers.size())
        return;
    layers[index]->opacity = opacity;
    markRegionDirty(0, 0, getWidth(), getHeight());
}

// --- PREVIEW LOGIC ---
void LayerManager::setPreviewPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    previewLayer->setPixel(x, y, r, g, b, a);
}

void LayerManager::addPreviewDirtyRect(int x, int y, int w, int h)
{
    int txStart = std::max(0, x / TILE_SIZE);
    int tyStart = std::max(0, y / TILE_SIZE);
    int txEnd = std::min(tilesX - 1, (x + w) / TILE_SIZE);
    int tyEnd = std::min(tilesY - 1, (y + h) / TILE_SIZE);

    for (int ty = tyStart; ty <= tyEnd; ++ty)
    {
        for (int tx = txStart; tx <= txEnd; ++tx)
        {
            previewDirtyTiles.insert(ty * tilesX + tx);
        }
    }
}

void LayerManager::clearPreview()
{
    for (int tileIndex : previewDirtyTiles)
    {
        int tx = tileIndex % tilesX;
        int ty = tileIndex / tilesX;
        markRegionDirty(tx * TILE_SIZE, ty * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    }

    previewLayer->clear();
    previewDirtyTiles.clear();
}

void LayerManager::showPreview()
{
    for (int tileIndex : previewDirtyTiles)
    {
        int tx = tileIndex % tilesX;
        int ty = tileIndex / tilesX;
        markRegionDirty(tx * TILE_SIZE, ty * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    }
}