#include "LayerManager.h"
#include <algorithm>
#include <iostream>

LayerManager::LayerManager(int width, int height)
    : width(width), height(height)
{
    previewLayer = std::make_shared<ColorLayer>(width, height, "Preview Layer");
    projectionCache.resize(width * height * 4, 0);
    addLayer("Background");

    tilesX = (width + TILE_SIZE - 1) / TILE_SIZE;
    tilesY = (height + TILE_SIZE - 1) / TILE_SIZE;

    layers.back()->fill(255, 255, 255, 255);
    markRegionDirty(0, 0, width, height);
}

void LayerManager::addLayer(const std::string &name, Layer::Type type)
{
    switch (type)
    {
    case Layer::Type::Color:
        layers.push_back(std::make_shared<ColorLayer>(width, height, name));
        break;
    case Layer::Type::Mask:
        layers.push_back(std::make_shared<MaskLayer>(width, height, name));
        break;
    default:
        layers.push_back(std::make_shared<ColorLayer>(width, height, name));
        break;
    }
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

void LayerManager::applyFilter(Filter &filter)
{
    // TODO: after adding the Mask layer update only the affected tiles instead of the whole layer
    if (activeLayerIndex >= layers.size())
        return;
    TiledLayer *activeLayer = layers[activeLayerIndex].get();

    // 1. Lock the drawing state
    beginBatch();

    // 2. Execute the pure math filter directly on the layer
    filter.apply(activeLayer);

    // 3. Force a full-screen redraw so the user sees the result
    addDirtyRect(0, 0, width, height);
    endBatch();
}

void LayerManager::updateCache(int startX, int startY, int rWidth, int rHeight)
{
    int x0 = std::max(0, startX);
    int y0 = std::max(0, startY);
    int x1 = std::min(width, startX + rWidth);
    int y1 = std::min(height, startY + rHeight);

    if (x1 <= x0 || y1 <= y0)
        return;

    for (int y = y0; y < y1; ++y)
    {
        int ty = y / TILE_SIZE;     // The Y coordinate on the tile grid
        int localY = y % TILE_SIZE; // The Y coordinate strictly inside the 64x64 tile

        for (int x = x0; x < x1;)
        {
            int tx = x / TILE_SIZE;     // The X coordinate on the tile grid
            int localX = x % TILE_SIZE; // The X coordinate strictly inside the 64x64 tile

            // Calculate how many pixels we can process before we cross into the next 64x64 tile
            int pixelsToProcess = std::min(x1 - x, TILE_SIZE - localX);

            // 1. GATHER PHASE: Get raw memory pointers for this specific 64x64 block
            std::vector<std::pair<const uint8_t *, float>> activeTiles;

            // Pre-allocate memory to avoid reallocations in the loop
            activeTiles.reserve(layers.size());

            for (const auto &layer : layers)
            {
                if (!layer->visible)
                    continue;

                // Grab the raw pointer. If the tile is empty, this returns nullptr.
                const Tile *tile = layer->getTile(tx, ty);

                if (tile != nullptr)
                {
                    activeTiles.push_back({tile->data.data(), layer->opacity});
                }
            }

            // 2. COMPOSITE PHASE: Loop through the chunk using the raw arrays
            for (int i = 0; i < pixelsToProcess; ++i)
            {
                int currentX = x + i;
                int globalIdx = (y * width + currentX) * 4;

                // Calculate the exact index inside the 64x64 flat tile vector
                int localTileIdx = (localY * TILE_SIZE + (localX + i)) * 4;

                uint8_t &outR = projectionCache[globalIdx];
                uint8_t &outG = projectionCache[globalIdx + 1];
                uint8_t &outB = projectionCache[globalIdx + 2];
                uint8_t &outA = projectionCache[globalIdx + 3];

                // Reset cache pixel to transparent
                outR = 0;
                outG = 0;
                outB = 0;
                outA = 0;

                for (const auto &activeTile : activeTiles)
                {
                    const uint8_t *tileData = activeTile.first;
                    float opacity = activeTile.second;

                    // Direct memory access. No virtual function overhead!
                    uint8_t tr = tileData[localTileIdx];
                    uint8_t tg = tileData[localTileIdx + 1];
                    uint8_t tb = tileData[localTileIdx + 2];
                    uint8_t ta = tileData[localTileIdx + 3];

                    if (ta > 0)
                    {
                        blendPixels(outR, outG, outB, outA, tr, tg, tb, ta, opacity);
                    }
                }
            }

            // 3. Jump forward by the chunk size we just processed
            x += pixelsToProcess;
        }
    }
}

void LayerManager::renderRegion(int startX, int startY, int rWidth, int rHeight, std::vector<uint8_t> &outBuffer) const
{
    int x0 = std::max(0, startX);
    int y0 = std::max(0, startY);
    int x1 = std::min(width, startX + rWidth);
    int y1 = std::min(height, startY + rHeight);

    int w = x1 - x0;
    int h = y1 - y0;

    if (w <= 0 || h <= 0)
        return;

    size_t requiredSize = w * h * 4;

    // 1. THE FIX: Only allocate memory if the buffer is too small.
    // If we call this every frame with the same dimensions, this does zero heap allocations!
    if (outBuffer.size() != requiredSize)
    {
        outBuffer.resize(requiredSize);
    }

    // 2. Copy the cached composited image row by row into our buffer
    for (int y = 0; y < h; ++y)
    {
        int globalIdx = ((y0 + y) * width + x0) * 4;
        int localIdx = (y * w) * 4;

        std::copy(projectionCache.begin() + globalIdx,
                  projectionCache.begin() + globalIdx + (w * 4),
                  outBuffer.begin() + localIdx);
    }

    // 3. Add the preview layer on top if it exists
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
                previewLayer->getPixel(globalX, globalY, tr, tg, tb, ta);

                if (ta > 0)
                {
                    uint8_t &br = outBuffer[localIdx];
                    uint8_t &bg = outBuffer[localIdx + 1];
                    uint8_t &bb = outBuffer[localIdx + 2];
                    uint8_t &ba = outBuffer[localIdx + 3];

                    blendPixels(br, bg, bb, ba, tr, tg, tb, ta, 1.0f);
                }
            }
        }
    }
}

void LayerManager::render(std::vector<uint8_t> &outBuffer) const
{
    renderRegion(0, 0, width, height, outBuffer);
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
    outA = (effectiveAlpha + (outA * invAlpha) / 255);
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