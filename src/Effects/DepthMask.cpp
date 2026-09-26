#include "DepthMask.h"
#include "../Core/ThreadPool.h"
#include "../Core/MathUtils.h"
#include <cmath>
#include <algorithm>

namespace AIDepthPro {

void DepthMask::GenerateMask(
    const DepthFrame& depth,
    std::vector<float>& outMask,
    float maskMin,
    float maskMax,
    float softness,
    float feather,
    bool invert
) {
    int w = depth.width;
    int h = depth.height;
    if (w <= 0 || h <= 0) return;

    outMask.resize(w * h, 0.0f);
    softness = std::max(softness, 1e-4f);

    ThreadPool::getInstance().parallelFor(0, h, [&](int y) {
        for (int x = 0; x < w; ++x) {
            float d = depth.get(x, y);

            // Compute soft range boundaries with smoothstep
            float lowEdge = Math::smoothstep(maskMin - softness, maskMin, d);
            float highEdge = 1.0f - Math::smoothstep(maskMax, maskMax + softness, d);

            float m = lowEdge * highEdge;

            if (invert) {
                m = 1.0f - m;
            }

            outMask[y * w + x] = Math::clamp(m, 0.0f, 1.0f);
        }
    });

    // Apply feather blur if feather > 0
    if (feather > 0.005f) {
        int radius = Math::clamp(static_cast<int>(feather * 30.0f), 1, 15);
        std::vector<float> temp(w * h, 0.0f);

        // Horizontal blur
        ThreadPool::getInstance().parallelFor(0, h, [&](int y) {
            for (int x = 0; x < w; ++x) {
                float sum = 0.0f;
                int count = 0;
                for (int dx = -radius; dx <= radius; ++dx) {
                    int nx = Math::clamp(x + dx, 0, w - 1);
                    sum += outMask[y * w + nx];
                    count++;
                }
                temp[y * w + x] = sum / count;
            }
        });

        // Vertical blur
        ThreadPool::getInstance().parallelFor(0, h, [&](int y) {
            for (int x = 0; x < w; ++x) {
                float sum = 0.0f;
                int count = 0;
                for (int dy = -radius; dy <= radius; ++dy) {
                    int ny = Math::clamp(y + dy, 0, h - 1);
                    sum += temp[ny * w + x];
                    count++;
                }
                outMask[y * w + x] = Math::clamp(sum / count, 0.0f, 1.0f);
            }
        });
    }
}

void DepthMask::ApplyMask(
    const ImageFrame& srcImage,
    const std::vector<float>& mask,
    ImageFrame& dstImage
) {
    int w = dstImage.width;
    int h = dstImage.height;
    if (w <= 0 || h <= 0 || !srcImage.isValid() || !dstImage.isValid() || mask.size() < static_cast<size_t>(w * h)) return;

    const float* srcFloat = static_cast<const float*>(srcImage.data);
    float* dstFloat = static_cast<float*>(dstImage.data);
    const uint8_t* srcByte = static_cast<const uint8_t*>(srcImage.data);
    uint8_t* dstByte = static_cast<uint8_t*>(dstImage.data);
    int numChannels = (dstImage.components == PixelComponent::RGBA) ? 4 : 3;

    ThreadPool::getInstance().parallelFor(0, h, [&](int y) {
        for (int x = 0; x < w; ++x) {
            float m = mask[y * w + x];

            if (dstImage.bitDepth == BitDepth::Float) {
                const float* s = (const float*)((const char*)srcFloat + y * srcImage.rowBytes) + x * numChannels;
                float* d = (float*)((char*)dstFloat + y * dstImage.rowBytes) + x * numChannels;

                d[0] = s[0] * m;
                d[1] = s[1] * m;
                d[2] = s[2] * m;
                if (numChannels == 4) d[3] = (srcImage.components == PixelComponent::RGBA ? s[3] : 1.0f) * m;
            } else {
                const uint8_t* s = (const uint8_t*)((const char*)srcByte + y * srcImage.rowBytes) + x * numChannels;
                uint8_t* d = (uint8_t*)((char*)dstByte + y * dstImage.rowBytes) + x * numChannels;

                d[0] = static_cast<uint8_t>(s[0] * m);
                d[1] = static_cast<uint8_t>(s[1] * m);
                d[2] = static_cast<uint8_t>(s[2] * m);
                if (numChannels == 4) d[3] = static_cast<uint8_t>((srcImage.components == PixelComponent::RGBA ? s[3] : 255) * m);
            }
        }
    });
}

} // namespace AIDepthPro
