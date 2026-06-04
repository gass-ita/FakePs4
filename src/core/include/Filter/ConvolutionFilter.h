#ifndef CONVOLUTIONFILTER_H
#define CONVOLUTIONFILTER_H
#include "Filter.h"
#include <algorithm>
#include <cmath>
#include <algorithm>

class ConvolutionFilter : public Filter
{
protected:
    std::vector<float> kernel;
    int kernelSize = 3;

    bool preserveAlpha = false;

    // Safe read using just the layer bounds!
    void getSafePixel(TiledLayer *layer, int x, int y, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a)
    {
        int cx = std::max(0, std::min(x, layer->width - 1));
        int cy = std::max(0, std::min(y, layer->height - 1));
        layer->getPixel(cx, cy, r, g, b, a);
    }

public:
    void apply(TiledLayer *layer) override;
};

class GaussianBlurFilter : public ConvolutionFilter
{
public:
    GaussianBlurFilter(int size = 5, float sigma = 1.0f);
};

// ==========================================
// CONVOLUTION FILTERS
// ==========================================

class SharpenFilter : public ConvolutionFilter
{
public:
    // intensity: 0.0 is no effect, 1.0 is standard, > 1.0 is extreme
    SharpenFilter(float intensity = 1.0f);
};

class EdgeDetectionFilter : public ConvolutionFilter
{
public:
    // intensity: 0.0 is black, 1.0 is standard edges, > 1.0 boosts faint edges
    EdgeDetectionFilter(float intensity = 1.0f);
};

class EmbossFilter : public ConvolutionFilter
{
public:
    // intensity: controls the depth of the 3D shadow effect
    EmbossFilter(float intensity = 1.0f);
};

// ==========================================
// POINT FILTERS
// ==========================================

class InvertFilter : public Filter
{
private:
    float amount; // Range: 0.0f (Original) to 1.0f (Fully Inverted)
public:
    InvertFilter(float blendAmount = 1.0f);
    void apply(TiledLayer *layer) override;
};

class GrayscaleFilter : public Filter
{
private:
    float amount; // Range: 0.0f (Full Color) to 1.0f (Pure Grayscale)
public:
    GrayscaleFilter(float blendAmount = 1.0f);
    void apply(TiledLayer *layer) override;
};

class BrightnessContrastFilter : public Filter
{
private:
    float brightness; // Range: -255 to +255
    float contrast;   // Range: -255 to +255
public:
    BrightnessContrastFilter(float brightnessLevel = 0.0f, float contrastLevel = 0.0f);
    void apply(TiledLayer *layer) override;
};

#endif // CONVOLUTIONFILTER_H