#include "DepthOfField.h"
#include "../Core/ThreadPool.h"
#include "../Core/MathUtils.h"
#include <cmath>
#include <vector>
#include <algorithm>

namespace AIDepthPro {

float DepthOfField::SampleFocusDepth(const DepthFrame& depth, const Point2D& focusPoint) {
    if (depth.width <= 0 || depth.height <= 0) return 0.5f;

    float u = Math::clamp(focusPoint.x, 0.0f, 1.0f);
    float v = Math::clamp(focusPoint.y, 0.0f, 1.0f);

    // Sample a small 5x5 neighborhood around the focus point and take the median for stability
    int cx = static_cast<int>(u * (depth.width - 1));
    int cy = static_cast<int>(v * (depth.height - 1));

    std::vector<float> samples;
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            samples.push_back(depth.get(cx + dx, cy + dy));
        }
    }
    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}

namespace {

// Pre-computed spiral / concentric bokeh sample points (normalized to radius 1.0)
struct BokehSample {
    float x;
    float y;
    float weight;
};

std::vector<BokehSample> generateBokehKernel(BokehShape shape, int numSamples = 32) {
    std::vector<BokehSample> samples;
    const float kPi = 3.14159265358979323846f;
    const float kGoldenAngle = 2.39996323f; // ~137.5 degrees

    for (int i = 0; i < numSamples; ++i) {
        float r = std::sqrt(static_cast<float>(i + 0.5f) / numSamples);
        float theta = i * kGoldenAngle;

        float x = r * std::cos(theta);
        float y = r * std::sin(theta);
        float w = 1.0f;

        if (shape == BokehShape::Hexagonal) {
            // Hexagon bounding test
            float hexAngle = std::fmod(theta + 2.0f * kPi, kPi / 3.0f) - (kPi / 6.0f);
            float hexR = r * std::cos(hexAngle);
            if (hexR > 0.866f) continue; // Outside hexagon boundary
        } else if (shape == BokehShape::Gaussian) {
            w = std::exp(-2.0f * r * r);
        }

        samples.push_back({ x, y, w });
    }
    return samples;
}

} // anonymous namespace

void DepthOfField::ApplyDoF(
    const ImageFrame& srcImage,
    const DepthFrame& depth,
    ImageFrame& dstImage,
    const EffectParams& params
) {
    int w = dstImage.width;
    int h = dstImage.height;
    if (w <= 0 || h <= 0 || !srcImage.isValid() || !dstImage.isValid()) return;

    float focusZ = params.autoSampleFocus ? SampleFocusDepth(depth, params.focusPoint) : params.focusDepth;
    float focusRange = std::max(params.focusRange, 0.005f);
    float maxBlur = Math::clamp(params.blurStrength, 0.0f, 64.0f);

    if (maxBlur <= 0.1f) {
        // Zero blur -> Direct Copy
        size_t copyBytes = size_t(w) * (srcImage.components == PixelComponent::RGBA ? 4 : 3) *
            (srcImage.bitDepth == BitDepth::Float ? sizeof(float) : 1);
        for (int y=0;y<h;++y) std::memcpy(static_cast<char*>(dstImage.data)+y*dstImage.rowBytes,
            static_cast<const char*>(srcImage.data)+y*srcImage.rowBytes,copyBytes);
        return;
    }

    int sampleCount = (params.quality == QualityMode::Fast) ? 16 : ((params.quality == QualityMode::Balanced) ? 32 : 48);
    std::vector<BokehSample> kernel = generateBokehKernel(params.bokehShape, sampleCount);

    const float* srcFloat = static_cast<const float*>(srcImage.data);
    float* dstFloat = static_cast<float*>(dstImage.data);
    const uint8_t* srcByte = static_cast<const uint8_t*>(srcImage.data);
    uint8_t* dstByte = static_cast<uint8_t*>(dstImage.data);
    int numChannels = (dstImage.components == PixelComponent::RGBA) ? 4 : 3;

    ThreadPool::getInstance().parallelFor(0, h, [&](int y) {
        for (int x = 0; x < w; ++x) {
            float centerZ = depth.get(x, y);
            float diff = centerZ - focusZ;

            // Differentiate foreground vs background blur
            float blurScale = (diff > 0.0f) ? params.fgBlur : params.bgBlur;
            float cocNorm = Math::clamp(std::abs(diff) / focusRange, 0.0f, 1.0f) * blurScale;
            float cocRadius = cocNorm * maxBlur;

            if (cocRadius < 0.5f) {
                // In sharp focus
                if (dstImage.bitDepth == BitDepth::Float) {
                    const float* s = (const float*)((const char*)srcFloat + y * srcImage.rowBytes) + x * numChannels;
                    float* d = (float*)((char*)dstFloat + y * dstImage.rowBytes) + x * numChannels;
                    d[0] = s[0]; d[1] = s[1]; d[2] = s[2];
                    if (numChannels == 4) d[3] = (srcImage.components == PixelComponent::RGBA ? s[3] : 1.0f);
                } else {
                    const uint8_t* s = (const uint8_t*)((const char*)srcByte + y * srcImage.rowBytes) + x * numChannels;
                    uint8_t* d = (uint8_t*)((char*)dstByte + y * dstImage.rowBytes) + x * numChannels;
                    d[0] = s[0]; d[1] = s[1]; d[2] = s[2];
                    if (numChannels == 4) d[3] = (srcImage.components == PixelComponent::RGBA ? s[3] : 255);
                }
                continue;
            }

            float accumR = 0.0f, accumG = 0.0f, accumB = 0.0f, accumA = 0.0f;
            float totalWeight = 0.0f;

            for (const auto& smp : kernel) {
                int sx = Math::clamp(static_cast<int>(std::round(x + smp.x * cocRadius)), 0, w - 1);
                int sy = Math::clamp(static_cast<int>(std::round(y + smp.y * cocRadius)), 0, h - 1);

                float sZ = depth.get(sx, sy);
                // Depth-aware weighting to prevent foreground background bleeding
                float sampleDiff = sZ - focusZ;
                float sampleCoc = Math::clamp(std::abs(sampleDiff) / focusRange, 0.0f, 1.0f) * maxBlur;
                float depthWeight = (sZ >= centerZ) ? 1.0f : Math::clamp(sampleCoc / std::max(cocRadius, 0.1f), 0.1f, 1.0f);

                float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
                if (srcImage.bitDepth == BitDepth::Float) {
                    const float* sp = (const float*)((const char*)srcFloat + sy * srcImage.rowBytes) + sx * numChannels;
                    r = sp[0]; g = sp[1]; b = sp[2];
                    if (numChannels == 4) a = sp[3];
                } else {
                    const uint8_t* sp = (const uint8_t*)((const char*)srcByte + sy * srcImage.rowBytes) + sx * numChannels;
                    r = sp[0] / 255.0f; g = sp[1] / 255.0f; b = sp[2] / 255.0f;
                    if (numChannels == 4) a = sp[3] / 255.0f;
                }

                // Highlight Boost / Specular Bloom
                if (params.highlightBoost > 0.01f) {
                    float lum = Math::rgbToLuminance(r, g, b);
                    if (lum > 0.65f) {
                        float boost = (lum - 0.65f) * params.highlightBoost * 3.0f;
                        r += boost;
                        g += boost;
                        b += boost;
                    }
                }

                float wFinal = smp.weight * depthWeight;
                accumR += r * wFinal;
                accumG += g * wFinal;
                accumB += b * wFinal;
                accumA += a * wFinal;
                totalWeight += wFinal;
            }

            if (totalWeight > 1e-5f) {
                accumR /= totalWeight;
                accumG /= totalWeight;
                accumB /= totalWeight;
                accumA /= totalWeight;
            }

            if (dstImage.bitDepth == BitDepth::Float) {
                float* d = (float*)((char*)dstFloat + y * dstImage.rowBytes) + x * numChannels;
                d[0] = accumR; d[1] = accumG; d[2] = accumB;
                if (numChannels == 4) d[3] = accumA;
            } else {
                uint8_t* d = (uint8_t*)((char*)dstByte + y * dstImage.rowBytes) + x * numChannels;
                d[0] = static_cast<uint8_t>(Math::clamp(accumR * 255.0f, 0.0f, 255.0f));
                d[1] = static_cast<uint8_t>(Math::clamp(accumG * 255.0f, 0.0f, 255.0f));
                d[2] = static_cast<uint8_t>(Math::clamp(accumB * 255.0f, 0.0f, 255.0f));
                if (numChannels == 4) d[3] = static_cast<uint8_t>(Math::clamp(accumA * 255.0f, 0.0f, 255.0f));
            }
        }
    });
}

} // namespace AIDepthPro
