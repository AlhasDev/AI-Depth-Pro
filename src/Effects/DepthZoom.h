#pragma once

#include "../Core/Types.h"

namespace AIDepthPro {

class DepthZoom {
public:
    // Apply depth-dependent differential zoom
    static void ApplyDepthZoom(
        const ImageFrame& srcImage,
        const DepthFrame& depth,
        ImageFrame& dstImage,
        const EffectParams& params
    );
};

} // namespace AIDepthPro
