#ifndef LAYER_MANAGER_H
#define LAYER_MANAGER_H

#include "Layer.h"
#include "LMObserver.h"
#include <vector>
#include <memory>
#include <cstdint>
#include <string>

class LayerManager
{
private:
    int width;
    int height;
    size_t activeLayerIndex = 0;

    std::vector<std::shared_ptr<Layer>> layers;
    std::vector<LMObserver *> observers;

    bool isBatching = false;
    int batchMinX = 0, batchMinY = 0, batchMaxX = 0, batchMaxY = 0;
    std::vector<uint8_t> projectionCache;

    // preview logic
    std::shared_ptr<Layer> previewLayer;
    int prevMinX = 0, prevMinY = 0;
    int prevMaxX = -1, prevMaxY = -1;

public:
    LayerManager(int width, int height);

    void addLayer(const std::string &name = "New Layer");

    void addObserver(LMObserver *observer);

    void markRegionDirty(int x, int y, int w, int h, bool skipCache = false);

    std::vector<uint8_t> render() const;

    std::vector<uint8_t> renderRegion(int startX, int startY, int regionWidth, int regionHeight) const;

    void setActiveLayer(size_t index);

    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    // Caching batching
    void beginBatch();
    void endBatch();

    // GETTERS for width and height
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    const std::vector<std::shared_ptr<Layer>> &getLayers() const { return layers; }
    size_t getActiveLayerIndex() const { return activeLayerIndex; }

    void setLayerVisibility(size_t index, bool visible);
    void setLayerName(size_t index, const std::string &name);
    void setLayerOpacity(size_t index, float opacity); // opacity as a float between 0.0 and 1.0

    // preview logic
    void clearPreview();

    void setPreviewPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void showPreview();

    // saving and loading
    bool saveProject(const std::string &filepath) const;
    bool loadProject(const std::string &filepath);

private:
    // Helper function for the alpha blending math
    void blendPixels(uint8_t &outR, uint8_t &outG, uint8_t &outB, uint8_t &outA,
                     uint8_t topR, uint8_t topG, uint8_t topB, uint8_t topA, float opacity) const;

    void updateCache(int x, int y, int w, int h);
};

#endif // LAYER_MANAGER_H