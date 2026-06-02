#include "LayerManager.h"
#include "ColorLayer.h"
#include "MaskLayer.h"
#include <fstream>
#include <vector>
#include <cstring>

bool LayerManager::saveProject(const std::string &filepath) const
{
    std::ofstream out(filepath, std::ios::binary);
    if (!out)
        return false;

    // 1. Header
    out.write("MGFX", 4);
    out.write(reinterpret_cast<const char *>(&width), sizeof(width));
    out.write(reinterpret_cast<const char *>(&height), sizeof(height));

    size_t numLayers = layers.size();
    out.write(reinterpret_cast<const char *>(&numLayers), sizeof(numLayers));

    // 2. Layers
    for (const auto &layer : layers)
    {
        size_t nameLen = layer->name.size();
        out.write(reinterpret_cast<const char *>(&nameLen), sizeof(nameLen));
        out.write(layer->name.data(), nameLen);

        // Store the type!
        uint8_t type = static_cast<uint8_t>(layer->getType());
        out.write(reinterpret_cast<const char *>(&type), sizeof(type));

        out.write(reinterpret_cast<const char *>(&layer->visible), sizeof(layer->visible));
        out.write(reinterpret_cast<const char *>(&layer->opacity), sizeof(layer->opacity));

        // Flatten pixels
        std::vector<uint8_t> flatData(width * height * 4, 0);
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
        out.write(reinterpret_cast<const char *>(flatData.data()), flatData.size());
    }
    return out.good();
}

bool LayerManager::loadProject(const std::string &filepath)
{
    std::ifstream in(filepath, std::ios::binary);
    if (!in)
        return false;

    char magic[4];
    in.read(magic, 4);
    if (std::strncmp(magic, "MGFX", 4) != 0)
        return false;

    int newWidth, newHeight;
    in.read(reinterpret_cast<char *>(&newWidth), sizeof(newWidth));
    in.read(reinterpret_cast<char *>(&newHeight), sizeof(newHeight));

    width = newWidth;
    height = newHeight;
    layers.clear();

    size_t numLayers;
    in.read(reinterpret_cast<char *>(&numLayers), sizeof(numLayers));

    for (size_t i = 0; i < numLayers; ++i)
    {
        size_t nameLen;
        in.read(reinterpret_cast<char *>(&nameLen), sizeof(nameLen));
        std::string name(nameLen, '\0');
        in.read(&name[0], nameLen);

        uint8_t type;
        in.read(reinterpret_cast<char *>(&type), sizeof(type));

        std::shared_ptr<Layer> newLayer;
        if (type == static_cast<uint8_t>(Layer::Type::Color))
        {
            newLayer = std::make_shared<ColorLayer>(width, height, name);
        }
        else if (type == static_cast<uint8_t>(Layer::Type::Mask))
        {
            newLayer = std::make_shared<MaskLayer>(width, height, name);
        }

        in.read(reinterpret_cast<char *>(&newLayer->visible), sizeof(newLayer->visible));
        in.read(reinterpret_cast<char *>(&newLayer->opacity), sizeof(newLayer->opacity));

        std::vector<uint8_t> flatData(width * height * 4);
        in.read(reinterpret_cast<char *>(flatData.data()), flatData.size());

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                int idx = (y * width + x) * 4;
                if (flatData[idx + 3] > 0)
                {
                    newLayer->setPixel(x, y, flatData[idx], flatData[idx + 1], flatData[idx + 2], flatData[idx + 3]);
                }
            }
        }
        layers.push_back(newLayer);
    }
    // set the whole canvas as dirty to force a full redraw
    markRegionDirty(0, 0, width, height);
    // update the observers for the new layers
    for (auto *obs : observers)
    {
        obs->onLayerListChanged();
    }
    return true;
}