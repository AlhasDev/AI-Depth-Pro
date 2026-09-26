#pragma once

#include "../Core/Types.h"

namespace AIDepthPro {

class EdgeRefinement {
public:
    // Fast O(N) Guided Filter for Depth Map edge refinement using RGB guide image
    static void ApplyGuidedFilter(
        const ImageFrame& rgbGuide,
        const DepthFrame& rawDepth,
        DepthFrame& refinedDepth,
        int radius = 4,
        float eps = 0.001f
    );

    // Fast Joint Bilateral Filter fallback
    static void ApplyJointBilateralFilter(
        const ImageFrame& rgbGuide,
        const DepthFrame& rawDepth,
        DepthFrame& refinedDepth,
        int radius = 3,
        float spatialSigma = 3.0f,
        float rangeSigma = 0.1f
    );
};

} // namespace AIDepthPro
