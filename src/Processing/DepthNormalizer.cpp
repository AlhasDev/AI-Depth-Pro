#include "DepthNormalizer.h"
#include "../Core/ThreadPool.h"
#include "../Core/MathUtils.h"
#include <cmath>

namespace AIDepthPro {

void DepthNormalizer::Process(
    const DepthFrame& inputDepth,
    DepthFrame& outputDepth,
    const EffectParams& params
) {
    int w = inputDepth.width;
    int h = inputDepth.height;
    if (w <= 0 || h <= 0) return;

    outputDepth.resize(w, h);

    float nearR = params.nearRange;
    float farR = params.farRange;
    if (std::abs(nearR - farR) < 1e-4f) farR = nearR + 0.001f;

    float invGamma = (params.depthGamma > 0.001f) ? (1.0f / params.depthGamma) : 1.0f;
    float contrast = params.depthContrast;
    float offset = params.depthOffset;
    float scale = params.depthScale;
    bool invert = params.invertDepth;

    ThreadPool::getInstance().parallelFor(0, h, [&](int y) {
        for (int x = 0; x < w; ++x) {
            float d = inputDepth.get(x, y);

            // 1. Remap near/far range
            d = (d - nearR) / (farR - nearR);
            d = Math::clamp(d, 0.0f, 1.0f);

            // 2. Invert if requested
            if (invert) {
                d = 1.0f - d;
            }

            // 3. Gamma
            if (std::abs(invGamma - 1.0f) > 1e-4f) {
                d = std::pow(d, invGamma);
            }

            // 4. Contrast & Offset & Scale
            d = (d - 0.5f) * contrast + 0.5f + offset;
            d *= scale;

            outputDepth.set(x, y, Math::clamp(d, 0.0f, 1.0f));
        }
    });
}

} // namespace AIDepthPro
