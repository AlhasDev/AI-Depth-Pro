#include "DepthZoom.h"
#include "../Core/ThreadPool.h"
#include "../Core/MathUtils.h"
#include <cmath>
#include <algorithm>

namespace AIDepthPro {

void DepthZoom::ApplyDepthZoom(
    const ImageFrame& srcImage,
    const DepthFrame& depth,
    ImageFrame& dstImage,
    const EffectParams& params
) {
    int w = dstImage.width;
    int h = dstImage.height;
    if (w <= 0 || h <= 0 || !srcImage.isValid() || !dstImage.isValid()) return;

    float cx = Math::clamp(params.zoomCenter.x, 0.0f, 1.0f);
    float cy = Math::clamp(params.zoomCenter.y, 0.0f, 1.0f);
    float nearS = std::max(params.nearScale, 0.01f);
    float farS = std::max(params.farScale, 0.01f);

    float* dstFloat = static_cast<float*>(dstImage.data);
    uint8_t* dstByte = static_cast<uint8_t*>(dstImage.data);
    const float* srcFloat = static_cast<const float*>(srcImage.data);
    const uint8_t* srcByte = static_cast<const uint8_t*>(srcImage.data);
    int numChannels = (dstImage.components == PixelComponent::RGBA) ? 4 : 3;

    ThreadPool::getInstance().parallelFor(0, h, [&](int y) {
        float normY = static_cast<float>(y) / (h > 1 ? (h - 1) : 1);

        for (int x = 0; x < w; ++x) {
            float normX = static_cast<float>(x) / (w > 1 ? (w - 1) : 1);

            float d = depth.sampleBilinear(normX, normY);
            float scale = Math::lerp(farS, nearS, d);
            if (scale < 0.001f) scale = 0.001f;

            float srcNormX = cx + (normX - cx) / scale;
            float srcNormY = cy + (normY - cy) / scale;

            // Bilinear sample source image
            srcNormX = Math::clamp(srcNormX, 0.0f, 1.0f);
            srcNormY = Math::clamp(srcNormY, 0.0f, 1.0f);

            float sx = srcNormX * (srcImage.width - 1);
            float sy = srcNormY * (srcImage.height - 1);
            int sx0 = static_cast<int>(sx);
            int sy0 = static_cast<int>(sy);
            int sx1 = std::min(sx0 + 1, srcImage.width - 1);
            int sy1 = std::min(sy0 + 1, srcImage.height - 1);
            float fx = sx - sx0;
            float fy = sy - sy0;

            float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;

            if (srcImage.bitDepth == BitDepth::Float) {
                const float* p00 = (const float*)((const char*)srcFloat + sy0 * srcImage.rowBytes) + sx0 * numChannels;
                const float* p10 = (const float*)((const char*)srcFloat + sy0 * srcImage.rowBytes) + sx1 * numChannels;
                const float* p01 = (const float*)((const char*)srcFloat + sy1 * srcImage.rowBytes) + sx0 * numChannels;
                const float* p11 = (const float*)((const char*)srcFloat + sy1 * srcImage.rowBytes) + sx1 * numChannels;

                r = (p00[0] * (1.0f - fx) + p10[0] * fx) * (1.0f - fy) + (p01[0] * (1.0f - fx) + p11[0] * fx) * fy;
                g = (p00[1] * (1.0f - fx) + p10[1] * fx) * (1.0f - fy) + (p01[1] * (1.0f - fx) + p11[1] * fx) * fy;
                b = (p00[2] * (1.0f - fx) + p10[2] * fx) * (1.0f - fy) + (p01[2] * (1.0f - fx) + p11[2] * fx) * fy;
                if (numChannels == 4) a = (p00[3] * (1.0f - fx) + p10[3] * fx) * (1.0f - fy) + (p01[3] * (1.0f - fx) + p11[3] * fx) * fy;

                float* dPixel = (float*)((char*)dstFloat + y * dstImage.rowBytes) + x * numChannels;
                dPixel[0] = r; dPixel[1] = g; dPixel[2] = b;
                if (numChannels == 4) dPixel[3] = a;
            } else {
                const uint8_t* p00 = (const uint8_t*)((const char*)srcByte + sy0 * srcImage.rowBytes) + sx0 * numChannels;
                const uint8_t* p10 = (const uint8_t*)((const char*)srcByte + sy0 * srcImage.rowBytes) + sx1 * numChannels;
                const uint8_t* p01 = (const uint8_t*)((const char*)srcByte + sy1 * srcImage.rowBytes) + sx0 * numChannels;
                const uint8_t* p11 = (const uint8_t*)((const char*)srcByte + sy1 * srcImage.rowBytes) + sx1 * numChannels;

                float r00 = p00[0] / 255.0f, g00 = p00[1] / 255.0f, b00 = p00[2] / 255.0f;
                float r10 = p10[0] / 255.0f, g10 = p10[1] / 255.0f, b10 = p10[2] / 255.0f;
                float r01 = p01[0] / 255.0f, g01 = p01[1] / 255.0f, b01 = p01[2] / 255.0f;
                float r11 = p11[0] / 255.0f, g11 = p11[1] / 255.0f, b11 = p11[2] / 255.0f;

                r = (r00 * (1.0f - fx) + r10 * fx) * (1.0f - fy) + (r01 * (1.0f - fx) + r11 * fx) * fy;
                g = (g00 * (1.0f - fx) + g10 * fx) * (1.0f - fy) + (g01 * (1.0f - fx) + g11 * fx) * fy;
                b = (b00 * (1.0f - fx) + b10 * fx) * (1.0f - fy) + (b01 * (1.0f - fx) + b11 * fx) * fy;

                if (numChannels == 4)
                    a = ((p00[3]*(1.0f-fx)+p10[3]*fx)*(1.0f-fy) +
                         (p01[3]*(1.0f-fx)+p11[3]*fx)*fy) / 255.0f;
                uint8_t* dPixel = (uint8_t*)((char*)dstByte + y * dstImage.rowBytes) + x * numChannels;
                dPixel[0] = static_cast<uint8_t>(Math::clamp(r * 255.0f, 0.0f, 255.0f));
                dPixel[1] = static_cast<uint8_t>(Math::clamp(g * 255.0f, 0.0f, 255.0f));
                dPixel[2] = static_cast<uint8_t>(Math::clamp(b * 255.0f, 0.0f, 255.0f));
                if (numChannels == 4) dPixel[3] = static_cast<uint8_t>(Math::clamp(a * 255.0f, 0.0f, 255.0f));
            }
        }
    });
}

} // namespace AIDepthPro
