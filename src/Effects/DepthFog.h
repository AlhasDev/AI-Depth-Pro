#pragma once

#include "../Core/Types.h"

namespace AIDepthPro {

class DepthFog {
public:
    // Apply depth-based atmospheric fog and haze
    static void ApplyFog(
        const ImageFrame& srcImage,
        const DepthFrame& depth,
        ImageFrame& dstImage,
        const EffectParams& params
    );
};

} // namespace AIDepthPro
