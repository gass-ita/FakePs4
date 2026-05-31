#include "LayerManager.h"
#include <fstream>
#include <cstring>

bool LayerManager::saveProject(const std::string &filepath) const
{
    // Open file in binary mode
    std::ofstream out(filepath, std::ios::binary);
    if (!out)
        return false;

    // 1. Write File Header
    out.write("MGFX", 4); // "Magic" signature
    out.write(reinterpret_cast<const char *>(&width), sizeof(width));
    out.write(reinterpret_cast<const char *>(&height), sizeof(height));

    size_t numLayers = layers.size();
    out.write(reinterpret_cast<const char *>(&numLayers), sizeof(numLayers));

    // 2. Write Layers
    for (const auto &layer : layers)
    {
        // Name
        size_t nameLen = layer->name.size();
        out.write(reinterpret_cast<const char *>(&nameLen), sizeof(nameLen));
        out.write(layer->name.data(), nameLen);

        // Metadata
        out.write(reinterpret_cast<const char *>(&layer->visible), sizeof(layer->visible));
        out.write(reinterpret_cast<const char *>(&layer->opacity), sizeof(layer->opacity));

        // Massive raw pixel dump (Blazing fast!)
        out.write(reinterpret_cast<const char *>(layer->pixels.data()), layer->pixels.size());
    }

    return out.good();
}

bool LayerManager::loadProject(const std::string &filepath)
{
    std::ifstream in(filepath, std::ios::binary);
    if (!in)
        return false;

    // 1. Verify Magic Signature
    char magic[4];
    in.read(magic, 4);
    if (std::strncmp(magic, "MGFX", 4) != 0)
        return false; // Not our file format!

    // 2. Read Canvas Dimensions
    int newWidth, newHeight;
    in.read(reinterpret_cast<char *>(&newWidth), sizeof(newWidth));
    in.read(reinterpret_cast<char *>(&newHeight), sizeof(newHeight));

    // Prepare engine for new dimensions
    width = newWidth;
    height = newHeight;
    layers.clear();                                // Destroy current layers
    projectionCache.assign(width * height * 4, 0); // Resize flat cache

    // 3. Read Layers
    size_t numLayers;
    in.read(reinterpret_cast<char *>(&numLayers), sizeof(numLayers));

    for (size_t i = 0; i < numLayers; ++i)
    {
        // Name
        size_t nameLen;
        in.read(reinterpret_cast<char *>(&nameLen), sizeof(nameLen));
        std::string name(nameLen, '\0');
        in.read(&name[0], nameLen);

        // Create new layer instance
        auto newLayer = std::make_shared<Layer>(width, height, name);

        // Metadata
        in.read(reinterpret_cast<char *>(&newLayer->visible), sizeof(newLayer->visible));
        in.read(reinterpret_cast<char *>(&newLayer->opacity), sizeof(newLayer->opacity));

        // Read raw pixel dump directly into vector
        in.read(reinterpret_cast<char *>(newLayer->pixels.data()), newLayer->pixels.size());

        layers.push_back(newLayer);
    }

    // Safely reset UI state
    activeLayerIndex = 0;

    // Wipe preview memory to prevent size mismatches
    previewLayer = std::make_shared<Layer>(width, height, "Preview Scratchpad");

    // Force complete canvas recalculation
    markRegionDirty(0, 0, width, height);

    for (auto *observer : observers)
    {
        observer->onLayerListChanged();
    }
    return true;
}