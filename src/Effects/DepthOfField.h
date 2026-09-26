#pragma once

#include "../Core/Types.h"

namespace AIDepthPro {

class DepthOfField {
public:
    // Sample depth at the focus point (normalized 0..1 coordinates)
    static float SampleFocusDepth(const DepthFrame& depth, const Point2D& focusPoint);

    // Apply high-quality Depth of Field with Bokeh simulation
    static void ApplyDoF(
        const ImageFrame& srcImage,
        const DepthFrame& depth,
        ImageFrame& dstImage,
        const EffectParams& params
    );
};

} // namespace AIDepthPro
