#include "ColorMap.h"
#include "../Core/MathUtils.h"
#include "../Core/ThreadPool.h"
#include <cmath>

namespace AIDepthPro {

void ColorMap::RenderVisualization(
    const ImageFrame& srcImage,
    const DepthFrame& depth,
    ImageFrame& dstImage,
    const EffectParams& params
) {
    int w = dstImage.width;
    int h = dstImage.height;
    if (w <= 0 || h <= 0 || !dstImage.isValid()) return;

    float* dstFloat = static_cast<float*>(dstImage.data);
    uint8_t* dstByte = static_cast<uint8_t*>(dstImage.data);

    const float* srcFloat = static_cast<const float*>(srcImage.data);
    const uint8_t* srcByte = static_cast<const uint8_t*>(srcImage.data);

    int numChannels = (dstImage.components == PixelComponent::RGBA) ? 4 : 3;

    ThreadPool::getInstance().parallelFor(0, h, [&](int y) {
        for (int x = 0; x < w; ++x) {
            float d = depth.get(x, y);
            ColorRGBA outColor = { 0.0f, 0.0f, 0.0f, 1.0f };

            switch (params.viewMode) {
                case ViewMode::Original: {
                    if (srcImage.isValid()) {
                        if (srcImage.bitDepth == BitDepth::Float) {
                            const float* s = (const float*)((const char*)srcFloat + y * srcImage.rowBytes) + x * numChannels;
                            outColor.r = s[0]; outColor.g = s[1]; outColor.b = s[2];
                            if (numChannels == 4) outColor.a = s[3];
                        } else {
                            const uint8_t* s = (const uint8_t*)((const char*)srcByte + y * srcImage.rowBytes) + x * numChannels;
                            outColor.r = s[0] / 255.0f; outColor.g = s[1] / 255.0f; outColor.b = s[2] / 255.0f;
                            if (numChannels == 4) outColor.a = s[3] / 255.0f;
                        }
                    }
                    break;
                }
                case ViewMode::DepthMap: {
                    outColor.r = d;
                    outColor.g = d;
                    outColor.b = d;
                    outColor.a = 1.0f;
                    break;
                }
                case ViewMode::InvertedDepth: {
                    float invD = 1.0f - d;
                    outColor.r = invD;
                    outColor.g = invD;
                    outColor.b = invD;
                    outColor.a = 1.0f;
                    break;
                }
                case ViewMode::NearFarMask: {
                    float m = (d >= params.maskMin && d <= params.maskMax) ? 1.0f : 0.0f;
                    outColor.r = m;
                    outColor.g = m;
                    outColor.b = m;
                    outColor.a = 1.0f;
                    break;
                }
                case ViewMode::FalseColor: {
                    outColor = Math::evaluateColorMap(d, params.colorMap);
                    break;
                }
                case ViewMode::EdgeMap: {
                    float dL = depth.get(x - 1, y);
                    float dR = depth.get(x + 1, y);
                    float dT = depth.get(x, y - 1);
                    float dB = depth.get(x, y + 1);

                    float gx = dR - dL;
                    float gy = dB - dT;
                    float edgeMag = Math::clamp(std::sqrt(gx * gx + gy * gy) * 4.0f, 0.0f, 1.0f);

                    outColor.r = edgeMag;
                    outColor.g = edgeMag;
                    outColor.b = edgeMag;
                    outColor.a = 1.0f;
                    break;
                }
            }

            // Write to destination buffer
            if (dstImage.bitDepth == BitDepth::Float) {
                float* dPixel = (float*)((char*)dstFloat + y * dstImage.rowBytes) + x * numChannels;
                dPixel[0] = outColor.r;
                dPixel[1] = outColor.g;
                dPixel[2] = outColor.b;
                if (numChannels == 4) dPixel[3] = outColor.a;
            } else {
                uint8_t* dPixel = (uint8_t*)((char*)dstByte + y * dstImage.rowBytes) + x * numChannels;
                dPixel[0] = static_cast<uint8_t>(Math::clamp(outColor.r * 255.0f, 0.0f, 255.0f));
                dPixel[1] = static_cast<uint8_t>(Math::clamp(outColor.g * 255.0f, 0.0f, 255.0f));
                dPixel[2] = static_cast<uint8_t>(Math::clamp(outColor.b * 255.0f, 0.0f, 255.0f));
                if (numChannels == 4) dPixel[3] = static_cast<uint8_t>(Math::clamp(outColor.a * 255.0f, 0.0f, 255.0f));
            }
        }
    });
}

} // namespace AIDepthPro
