#include "LayerManager.h"
#include <algorithm>
#include <iostream>

LayerManager::LayerManager(int width, int height)
    : width(width), height(height)
{
    previewLayer = std::make_shared<Layer>(width, height, "Preview Layer");
    projectionCache.resize(width * height * 4, 0);
    addLayer("Background");
    // Start with a solid white background
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

void LayerManager::markRegionDirty(int x, int y, int w, int h)
{
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

// Modify your existing setPixel:
void LayerManager::setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (isPreviewMode)
    {
        previewLayer->setPixel(x, y, r, g, b, a);
    }
    else
    {
        if (activeLayerIndex >= layers.size())
            return;
        layers[activeLayerIndex]->setPixel(x, y, r, g, b, a);
    }

    if (isBatching)
    {
        // Expand the dirty bounding box silently
        batchMinX = std::min(batchMinX, x);
        batchMinY = std::min(batchMinY, y);
        batchMaxX = std::max(batchMaxX, x);
        batchMaxY = std::max(batchMaxY, y);
    }
    else
    {
        // Normal immediate update
        markRegionDirty(x, y, 1, 1);
    }
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

            // 1. Reset this cache pixel to completely transparent (0)
            projectionCache[globalIdx] = 0;
            projectionCache[globalIdx + 1] = 0;
            projectionCache[globalIdx + 2] = 0;
            projectionCache[globalIdx + 3] = 0;

            // 2. Re-composite all layers for this specific pixel
            for (const auto &layer : layers)
            {
                if (!layer->visible)
                    continue;

                uint8_t &br = projectionCache[globalIdx];
                uint8_t &bg = projectionCache[globalIdx + 1];
                uint8_t &bb = projectionCache[globalIdx + 2];
                uint8_t &ba = projectionCache[globalIdx + 3];

                uint8_t tr = layer->pixels[globalIdx];
                uint8_t tg = layer->pixels[globalIdx + 1];
                uint8_t tb = layer->pixels[globalIdx + 2];
                uint8_t ta = layer->pixels[globalIdx + 3];

                blendPixels(br, bg, bb, ba, tr, tg, tb, ta, layer->opacity);
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

    // Blend the preview layer on top of the region buffer
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            int localIdx = (y * w + x) * 4;
            int globalIdx = ((y0 + y) * width + (x0 + x)) * 4;

            uint8_t &br = regionBuffer[localIdx];
            uint8_t &bg = regionBuffer[localIdx + 1];
            uint8_t &bb = regionBuffer[localIdx + 2];
            uint8_t &ba = regionBuffer[localIdx + 3];

            uint8_t tr = previewLayer->pixels[globalIdx];
            uint8_t tg = previewLayer->pixels[globalIdx + 1];
            uint8_t tb = previewLayer->pixels[globalIdx + 2];
            uint8_t ta = previewLayer->pixels[globalIdx + 3];

            blendPixels(br, bg, bb, ba, tr, tg, tb, ta, 1.0f);
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
    // Reset bounding box to extreme opposites
    batchMinX = width;
    batchMinY = height;
    batchMaxX = 0;
    batchMaxY = 0;
}

void LayerManager::endBatch()
{
    isBatching = false;
    // Only update if something was actually drawn
    if (batchMinX <= batchMaxX && batchMinY <= batchMaxY)
    {
        markRegionDirty(batchMinX, batchMinY, batchMaxX - batchMinX + 1, batchMaxY - batchMinY + 1);
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

        // CRITICAL: If a layer disappears or appears, the entire cache is invalid.
        // We must tell the observer to redraw the entire width and height of the canvas.
        markRegionDirty(0, 0, width, height);
    }
}

void LayerManager::setLayerName(size_t index, const std::string &name)
{
    if (index >= layers.size())
        return;
    // Assuming your Layer class has a public 'name' string property
    // If not, you may need to add it to Layer.h!
    layers[index]->name = name;
}

void LayerManager::setPreviewMode(bool active)
{
    isPreviewMode = active;
}

void LayerManager::clearPreview()
{
    previewLayer->clear();
    markRegionDirty(0, 0, width, height);
}