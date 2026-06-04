#include "ConvolutionFilter.h"

GaussianBlurFilter::GaussianBlurFilter(int size, float sigma)
{
    // 1. Safety Checks
    // Force size to be an odd number (e.g., 4 becomes 5) so we have a true center pixel
    kernelSize = (size % 2 == 0) ? size + 1 : size;

    // Prevent Division by Zero if someone passes sigma = 0
    sigma = std::max(0.0001f, sigma);

    int offset = kernelSize / 2;
    kernel.resize(kernelSize * kernelSize);

    float sum = 0.0f;
    int i = 0;

    // 2. Calculate the raw Gaussian Bell Curve weights
    for (int y = -offset; y <= offset; ++y)
    {
        for (int x = -offset; x <= offset; ++x)
        {
            // We don't need the full complex formula because we normalize at the end.
            // We just need the core exponent math.
            float weight = std::exp(-(x * x + y * y) / (2.0f * sigma * sigma));

            kernel[i++] = weight;
            sum += weight;
        }
    }

    // 3. Normalize the kernel (Conservation of Light)
    for (size_t j = 0; j < kernel.size(); ++j)
    {
        kernel[j] /= sum;
    }
}