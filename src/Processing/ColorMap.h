#pragma once

#include "../Core/Types.h"

namespace AIDepthPro {

class ColorMap {
public:
    static void RenderVisualization(
        const ImageFrame& srcImage,
        const DepthFrame& depth,
        ImageFrame& dstImage,
        const EffectParams& params
    );
};

} // namespace AIDepthPro
