#pragma once

#include "../Core/Types.h"
#include "../Core/MathUtils.h"
#include <vector>

namespace AIDepthPro {

class Relighting {
public:
    // Compute surface normal map from depth frame gradients
    static void ComputeNormals(
        const DepthFrame& depth,
        std::vector<Math::Vec3>& outNormals,
        float normalStrength = 2.0f
    );

    // Apply scene relighting using reconstructed surface normals and Phong illumination
    static void ApplyRelighting(
        const ImageFrame& srcImage,
        const DepthFrame& depth,
        ImageFrame& dstImage,
        const EffectParams& params
    );
};

} // namespace AIDepthPro
