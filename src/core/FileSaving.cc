#include "LayerManager.h"
#include <fstream>
#include <cstring>
#include <vector>

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

        // --- NEW TILE-SAFE SAVING ---
        // 1. Create a temporary flat buffer for the hard drive
        std::vector<uint8_t> flatData(width * height * 4, 0);

        // 2. Safely extract the pixels from the sparse tiles
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                uint8_t r, g, b, a;
                layer->getPixel(x, y, r, g, b, a);

                int idx = (y * width + x) * 4;
                flatData[idx] = r;
                flatData[idx + 1] = g;
                flatData[idx + 2] = b;
                flatData[idx + 3] = a;
            }
        }

        // 3. Dump the flat buffer to the file (Maintains backward compatibility!)
        out.write(reinterpret_cast<const char *>(flatData.data()), flatData.size());
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

    // Calculate how many bytes each layer's flat array should be
    size_t layerBytes = width * height * 4;

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

        // --- NEW TILE-SAFE LOADING ---
        // 1. Read the giant flat array from the hard drive into temporary memory
        std::vector<uint8_t> flatData(layerBytes);
        in.read(reinterpret_cast<char *>(flatData.data()), layerBytes);

        // 2. Reconstruct the sparse tiles!
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                int idx = (y * width + x) * 4;

                uint8_t a = flatData[idx + 3];

                // THE MAGIC TRICK: Only call setPixel (which allocates RAM) if the pixel actually exists!
                if (a > 0)
                {
                    uint8_t r = flatData[idx];
                    uint8_t g = flatData[idx + 1];
                    uint8_t b = flatData[idx + 2];
                    newLayer->setPixel(x, y, r, g, b, a);
                }
            }
        }

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