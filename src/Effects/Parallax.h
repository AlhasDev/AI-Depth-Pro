#pragma once

#include "../Core/Types.h"

namespace AIDepthPro {

class Parallax {
public:
    // Apply 3D Parallax displacement and smart edge filling
    static void ApplyParallax(
        const ImageFrame& srcImage,
        const DepthFrame& depth,
        ImageFrame& dstImage,
        const EffectParams& params
    );
};

} // namespace AIDepthPro
