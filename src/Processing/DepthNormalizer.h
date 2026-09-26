#pragma once

#include "../Core/Types.h"

namespace AIDepthPro {

class DepthNormalizer {
public:
    static void Process(
        const DepthFrame& inputDepth,
        DepthFrame& outputDepth,
        const EffectParams& params
    );
};

} // namespace AIDepthPro
