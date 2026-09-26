#pragma once

#include "../Core/Types.h"
#include <vector>

namespace AIDepthPro {

class DepthMask {
public:
    // Generate a normalized [0..1] float mask from depth buffer based on range, softness and feather
    static void GenerateMask(
        const DepthFrame& depth,
        std::vector<float>& outMask,
        float maskMin,
        float maskMax,
        float softness,
        float feather,
        bool invert
    );

    // Apply mask to composite source image and background or isolate layer
    static void ApplyMask(
        const ImageFrame& srcImage,
        const std::vector<float>& mask,
        ImageFrame& dstImage
    );
};

} // namespace AIDepthPro
