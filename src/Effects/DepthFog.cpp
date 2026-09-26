#include "DepthFog.h"
#include "../Core/ThreadPool.h"
#include "../Core/MathUtils.h"
#include <cmath>
#include <algorithm>

namespace AIDepthPro {

void DepthFog::ApplyFog(
    const ImageFrame& srcImage,
    const DepthFrame& depth,
    ImageFrame& dstImage,
    const EffectParams& params
) {
    int w = dstImage.width;
    int h = dstImage.height;
    if (w <= 0 || h <= 0 || !srcImage.isValid() || !dstImage.isValid()) return;

    float fogAmount = Math::clamp(params.fogAmount, 0.0f, 1.0f);
    float fogStart = Math::clamp(params.fogStart, 0.0f, 1.0f);
    float fogEnd = Math::clamp(params.fogEnd, 0.0f, 1.0f);
    if (std::abs(fogEnd - fogStart) < 1e-4f) fogEnd = fogStart + 0.001f;

    float falloff = std::max(params.fogFalloff, 0.1f);
    ColorRGBA fogColor = params.fogColor;

    float* dstFloat = static_cast<float*>(dstImage.data);
    uint8_t* dstByte = static_cast<uint8_t*>(dstImage.data);
    const float* srcFloat = static_cast<const float*>(srcImage.data);
    const uint8_t* srcByte = static_cast<const uint8_t*>(srcImage.data);
    int numChannels = (dstImage.components == PixelComponent::RGBA) ? 4 : 3;

    ThreadPool::getInstance().parallelFor(0, h, [&](int y) {
        for (int x = 0; x < w; ++x) {
            float d = depth.get(x, y);

            // Distance metric: 0.0 = closest to camera, 1.0 = farthest horizon
            float distance = 1.0f - d;

            float fogNorm = (distance - fogStart) / (fogEnd - fogStart);
            fogNorm = Math::clamp(fogNorm, 0.0f, 1.0f);

            float fogFactor = fogAmount * std::pow(fogNorm, falloff);
            fogFactor = Math::clamp(fogFactor, 0.0f, 1.0f);

            float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;

            if (srcImage.bitDepth == BitDepth::Float) {
                const float* s = (const float*)((const char*)srcFloat + y * srcImage.rowBytes) + x * numChannels;
                r = s[0]; g = s[1]; b = s[2];
                if (numChannels == 4) a = s[3];
            } else {
                const uint8_t* s = (const uint8_t*)((const char*)srcByte + y * srcImage.rowBytes) + x * numChannels;
                r = s[0] / 255.0f; g = s[1] / 255.0f; b = s[2] / 255.0f;
                if (numChannels == 4) a = s[3] / 255.0f;
            }

            // Atmospheric additive/blended scattering
            float outR = r * (1.0f - fogFactor) + fogColor.r * fogFactor;
            float outG = g * (1.0f - fogFactor) + fogColor.g * fogFactor;
            float outB = b * (1.0f - fogFactor) + fogColor.b * fogFactor;

            if (dstImage.bitDepth == BitDepth::Float) {
                float* dPixel = (float*)((char*)dstFloat + y * dstImage.rowBytes) + x * numChannels;
                dPixel[0] = outR; dPixel[1] = outG; dPixel[2] = outB;
                if (numChannels == 4) dPixel[3] = a;
            } else {
                uint8_t* dPixel = (uint8_t*)((char*)dstByte + y * dstImage.rowBytes) + x * numChannels;
                dPixel[0] = static_cast<uint8_t>(Math::clamp(outR * 255.0f, 0.0f, 255.0f));
                dPixel[1] = static_cast<uint8_t>(Math::clamp(outG * 255.0f, 0.0f, 255.0f));
                dPixel[2] = static_cast<uint8_t>(Math::clamp(outB * 255.0f, 0.0f, 255.0f));
                if (numChannels == 4) dPixel[3] = static_cast<uint8_t>(Math::clamp(a * 255.0f, 0.0f, 255.0f));
            }
        }
    });
}

} // namespace AIDepthPro
