#include "ConvolutionFilter.h"
#include <vector>

void ConvolutionFilter::apply(TiledLayer *layer)
{
    if (!layer)
        return;

    int width = layer->width;
    int height = layer->height;
    int offset = kernelSize / 2;

    std::vector<uint8_t> resultBuffer(width * height * 4, 0);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            // Grab the original center pixel so we know its true Alpha!
            uint8_t centerR, centerG, centerB, centerA;
            getSafePixel(layer, x, y, centerR, centerG, centerB, centerA);

            float sumR = 0, sumG = 0, sumB = 0, sumA = 0;
            int kernelIdx = 0;

            for (int ky = -offset; ky <= offset; ++ky)
            {
                for (int kx = -offset; kx <= offset; ++kx)
                {
                    uint8_t r, g, b, a;
                    getSafePixel(layer, x + kx, y + ky, r, g, b, a);

                    float weight = kernel[kernelIdx++];
                    sumR += r * weight;
                    sumG += g * weight;
                    sumB += b * weight;
                    sumA += a * weight;
                }
            }

            if (preserveAlpha)
            {
                sumA = static_cast<float>(centerA);
            }

            int bufferIdx = (y * width + x) * 4;
            resultBuffer[bufferIdx] = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, sumR)));
            resultBuffer[bufferIdx + 1] = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, sumG)));
            resultBuffer[bufferIdx + 2] = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, sumB)));
            resultBuffer[bufferIdx + 3] = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, sumA)));
        }
    }

    // Swap the temporary buffer back into the permanent layer memory
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int bufferIdx = (y * width + x) * 4;
            uint8_t a = resultBuffer[bufferIdx + 3];

            if (a > 0)
            {
                layer->setPixel(x, y, resultBuffer[bufferIdx], resultBuffer[bufferIdx + 1], resultBuffer[bufferIdx + 2], a);
            }
            else
            {
                layer->setPixel(x, y, 0, 0, 0, 0);
            }
        }
    }
}

// ==========================================
// CONVOLUTION KERNELS
// ==========================================

SharpenFilter::SharpenFilter(float intensity)
{
    kernelSize = 3;
    preserveAlpha = true; // Keep the original alpha so we don't create black smudges around transparent areas
    // We maintain a sum of 1.0 by making sure the center pixel perfectly balances the negative edges.
    // Base: -intensity, Center: (4 * intensity) + 1.0
    kernel = {
        0.0f, -intensity, 0.0f,
        -intensity, (4.0f * intensity) + 1.0f, -intensity,
        0.0f, -intensity, 0.0f};
}

EdgeDetectionFilter::EdgeDetectionFilter(float intensity)
{
    kernelSize = 3;
    preserveAlpha = true; // Keep the original alpha so edges show up on transparent areas instead of creating black smudges
    // The sum must remain 0.0 so flat colors become black.
    kernel = {
        -intensity, -intensity, -intensity,
        -intensity, 8.0f * intensity, -intensity,
        -intensity, -intensity, -intensity};
}

EmbossFilter::EmbossFilter(float intensity)
{
    kernelSize = 3;
    preserveAlpha = true; // Keep the original alpha so the emboss effect shows up on transparent areas instead of creating black smudges
    // The sum remains 1.0 (the center pixel) while the shadow/highlight edges cancel each other out.
    kernel = {
        -2.0f * intensity, -1.0f * intensity, 0.0f,
        -1.0f * intensity, 1.0f, 1.0f * intensity,
        0.0f, 1.0f * intensity, 2.0f * intensity};
}

// ==========================================
// POINT OPERATIONS
// ==========================================

InvertFilter::InvertFilter(float blendAmount)
{
    amount = std::max(0.0f, std::min(1.0f, blendAmount));
}

void InvertFilter::apply(TiledLayer *layer)
{
    if (!layer || amount <= 0.0f)
        return;

    for (int y = 0; y < layer->height; ++y)
    {
        for (int x = 0; x < layer->width; ++x)
        {
            uint8_t r, g, b, a;
            layer->getPixel(x, y, r, g, b, a);

            if (a > 0)
            {
                // target = 255 - current
                // Lerp formula: current + amount * (target - current)
                float newR = r + amount * ((255.0f - r) - r);
                float newG = g + amount * ((255.0f - g) - g);
                float newB = b + amount * ((255.0f - b) - b);

                layer->setPixel(x, y,
                                static_cast<uint8_t>(newR),
                                static_cast<uint8_t>(newG),
                                static_cast<uint8_t>(newB),
                                a);
            }
        }
    }
}

GrayscaleFilter::GrayscaleFilter(float blendAmount)
{
    amount = std::max(0.0f, std::min(1.0f, blendAmount));
}

void GrayscaleFilter::apply(TiledLayer *layer)
{
    if (!layer || amount <= 0.0f)
        return;

    for (int y = 0; y < layer->height; ++y)
    {
        for (int x = 0; x < layer->width; ++x)
        {
            uint8_t r, g, b, a;
            layer->getPixel(x, y, r, g, b, a);

            if (a > 0)
            {
                float gray = (r * 0.299f) + (g * 0.587f) + (b * 0.114f);

                // Lerp towards the gray value based on the amount slider
                float newR = r + amount * (gray - r);
                float newG = g + amount * (gray - g);
                float newB = b + amount * (gray - b);

                layer->setPixel(x, y,
                                static_cast<uint8_t>(newR),
                                static_cast<uint8_t>(newG),
                                static_cast<uint8_t>(newB),
                                a);
            }
        }
    }
}

BrightnessContrastFilter::BrightnessContrastFilter(float brightnessLevel, float contrastLevel)
{
    brightness = std::max(-255.0f, std::min(255.0f, brightnessLevel));
    contrast = std::max(-255.0f, std::min(255.0f, contrastLevel));
}

void BrightnessContrastFilter::apply(TiledLayer *layer)
{
    if (!layer)
        return;

    float factor = (259.0f * (contrast + 255.0f)) / (255.0f * (259.0f - contrast));

    for (int y = 0; y < layer->height; ++y)
    {
        for (int x = 0; x < layer->width; ++x)
        {
            uint8_t r, g, b, a;
            layer->getPixel(x, y, r, g, b, a);

            if (a > 0)
            {
                float newR = factor * (r - 128.0f) + 128.0f + brightness;
                float newG = factor * (g - 128.0f) + 128.0f + brightness;
                float newB = factor * (b - 128.0f) + 128.0f + brightness;

                layer->setPixel(x, y,
                                static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, newR))),
                                static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, newG))),
                                static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, newB))),
                                a);
            }
        }
    }
}