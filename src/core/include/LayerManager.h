#ifndef LAYER_MANAGER_H
#define LAYER_MANAGER_H

#include "ColorLayer.h"
#include "MaskLayer.h"
#include "LMObserver.h"
#include <vector>
#include <memory>
#include <cstdint>
#include <string>
#include <unordered_set>
#include "Filter.h"

class Tool; // Forward declaration to avoid circular dependency

class LayerManager
{
public:
    struct DirtyRect
    {
        int x, y, width, height;
    };

private:
    int width;
    int height;
    size_t activeLayerIndex = 0;

    std::vector<std::shared_ptr<TiledLayer>> layers;
    std::vector<LMObserver *> observers;

    bool isBatching = false;
    std::vector<uint8_t> projectionCache;
    int tilesX, tilesY;
    std::unordered_set<int> dirtyTiles;

    // tool settings
    uint8_t r = 0, g = 0, b = 0, a = 255;
    int currentToolSize = 1;
    std::unique_ptr<Tool> activeTool;

    // preview logic
    std::shared_ptr<ColorLayer> previewLayer;
    struct PreviewBBox
    {
        int x = 0, y = 0, w = 0, h = 0;
        bool valid = false;
    };
    PreviewBBox previewBBox;

public:
    LayerManager(int width, int height);

    void addLayer(const std::string &name = "New Layer", Layer::Type type = Layer::Type::Color);

    void addObserver(LMObserver *observer);

    // events
    void onMove(int x, int y, float pressure, float tiltX, float tiltY);
    void onPress(int x, int y, float pressure, float tiltX, float tiltY);
    void onRelease(int x, int y, float pressure, float tiltX, float tiltY);
    void onHover(int x, int y);

    void markRegionDirty(int x, int y, int w, int h, bool skipCache = false);

    void renderRegion(int startX, int startY, int rWidth, int rHeight, std::vector<uint8_t> &outBuffer) const;
    void render(std::vector<uint8_t> &outBuffer) const;

    void setActiveLayer(size_t index);

    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void getPixel(int x, int y, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) const;
    void applyFilter(Filter &filter); // Add this public method

    // Caching batching
    void beginBatch();
    void endBatch();
    void addDirtyRect(int x, int y, int w, int h);

    // GETTERS for width and height
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    const std::vector<std::shared_ptr<TiledLayer>> &getLayers() const { return layers; }
    size_t getActiveLayerIndex() const { return activeLayerIndex; }

    void setLayerVisibility(size_t index, bool visible);
    void setLayerName(size_t index, const std::string &name);
    void setLayerOpacity(size_t index, float opacity); // opacity as a float between 0.0 and 1.0

    void setActiveTool(std::unique_ptr<Tool> tool);

    Tool *getActiveTool();
    void setToolColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255);
    void setToolSize(int newSize);

    // preview logic
    void clearPreview();

    void setPreviewPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void showPreview();
    void addPreviewDirtyRect(int x, int y, int w, int h);

    // saving and loading
    bool saveProject(const std::string &filepath) const;
    bool loadProject(const std::string &filepath);
    // A simple struct to hold rectangle data
    void fillPreviewSpan(int x, int y, int length, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

private:
    // Helper function for the alpha blending math
    void blendPixels(uint8_t &outR, uint8_t &outG, uint8_t &outB, uint8_t &outA,
                     uint8_t topR, uint8_t topG, uint8_t topB, uint8_t topA, float opacity) const;

    void updateCache(int x, int y, int w, int h);
};

#endif // LAYER_MANAGER_H