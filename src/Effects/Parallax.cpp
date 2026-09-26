#include "Parallax.h"
#include "../Core/ThreadPool.h"
#include "../Core/MathUtils.h"
#include <cmath>
#include <algorithm>

namespace AIDepthPro {

namespace {

ColorRGBA sampleImage(
    const ImageFrame& img,
    float u,
    float v,
    EdgeFillMode mode
) {
    int w = img.width;
    int h = img.height;
    int numChannels = (img.components == PixelComponent::RGBA) ? 4 : 3;

    bool isOutside = (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f);

    if (isOutside) {
        if (mode == EdgeFillMode::Transparent) {
            return { 0.0f, 0.0f, 0.0f, 0.0f };
        } else if (mode == EdgeFillMode::Mirror) {
            // Mirror repeat
            float mu = std::fmod(std::abs(u), 2.0f);
            if (mu > 1.0f) mu = 2.0f - mu;
            float mv = std::fmod(std::abs(v), 2.0f);
            if (mv > 1.0f) mv = 2.0f - mv;
            u = mu;
            v = mv;
        } else {
            // Repeat / Clamp / Inpaint base
            u = Math::clamp(u, 0.0f, 1.0f);
            v = Math::clamp(v, 0.0f, 1.0f);
        }
    }

    float x = u * (w - 1);
    float y = v * (h - 1);
    int x0 = static_cast<int>(x);
    int y0 = static_cast<int>(y);
    int x1 = std::min(x0 + 1, w - 1);
    int y1 = std::min(y0 + 1, h - 1);
    float fx = x - x0;
    float fy = y - y0;

    auto getPixel = [&](int px, int py) -> ColorRGBA {
        if (img.bitDepth == BitDepth::Float) {
            const float* p = (const float*)((const char*)img.data + py * img.rowBytes) + px * numChannels;
            return { p[0], p[1], p[2], (numChannels == 4 ? p[3] : 1.0f) };
        } else {
            const uint8_t* p = (const uint8_t*)((const char*)img.data + py * img.rowBytes) + px * numChannels;
            return { p[0] / 255.0f, p[1] / 255.0f, p[2] / 255.0f, (numChannels == 4 ? p[3] / 255.0f : 1.0f) };
        }
    };

    ColorRGBA c00 = getPixel(x0, y0);
    ColorRGBA c10 = getPixel(x1, y0);
    ColorRGBA c01 = getPixel(x0, y1);
    ColorRGBA c11 = getPixel(x1, y1);

    ColorRGBA c0 = {
        c00.r * (1.0f - fx) + c10.r * fx,
        c00.g * (1.0f - fx) + c10.g * fx,
        c00.b * (1.0f - fx) + c10.b * fx,
        c00.a * (1.0f - fx) + c10.a * fx
    };
    ColorRGBA c1 = {
        c01.r * (1.0f - fx) + c11.r * fx,
        c01.g * (1.0f - fx) + c11.g * fx,
        c01.b * (1.0f - fx) + c11.b * fx,
        c01.a * (1.0f - fx) + c11.a * fx
    };

    return {
        c0.r * (1.0f - fy) + c1.r * fy,
        c0.g * (1.0f - fy) + c1.g * fy,
        c0.b * (1.0f - fy) + c1.b * fy,
        c0.a * (1.0f - fy) + c1.a * fy
    };
}

} // anonymous namespace

void Parallax::ApplyParallax(
    const ImageFrame& srcImage,
    const DepthFrame& depth,
    ImageFrame& dstImage,
    const EffectParams& params
) {
    int w = dstImage.width;
    int h = dstImage.height;
    if (w <= 0 || h <= 0 || !srcImage.isValid() || !dstImage.isValid()) return;

    float camDist = std::max(params.cameraDistance, 0.1f);
    float shiftScaleX = (params.parallaxX * params.depthStrength) / camDist;
    float shiftScaleY = (params.parallaxY * params.depthStrength) / camDist;

    float* dstFloat = static_cast<float*>(dstImage.data);
    uint8_t* dstByte = static_cast<uint8_t*>(dstImage.data);
    int numChannels = (dstImage.components == PixelComponent::RGBA) ? 4 : 3;

    ThreadPool::getInstance().parallelFor(0, h, [&](int y) {
        float normY = static_cast<float>(y) / (h > 1 ? (h - 1) : 1);

        for (int x = 0; x < w; ++x) {
            float normX = static_cast<float>(x) / (w > 1 ? (w - 1) : 1);

            // Sample depth at current destination coordinate
            float d = depth.sampleBilinear(normX, normY);

            // Forward-backward parallax displacement offset
            float offsetD = d - 0.5f;
            float srcNormX = normX - shiftScaleX * offsetD;
            float srcNormY = normY - shiftScaleY * offsetD;

            ColorRGBA c = sampleImage(srcImage, srcNormX, srcNormY, params.edgeFill);

            // Write to destination
            if (dstImage.bitDepth == BitDepth::Float) {
                float* dPixel = (float*)((char*)dstFloat + y * dstImage.rowBytes) + x * numChannels;
                dPixel[0] = c.r;
                dPixel[1] = c.g;
                dPixel[2] = c.b;
                if (numChannels == 4) dPixel[3] = c.a;
            } else {
                uint8_t* dPixel = (uint8_t*)((char*)dstByte + y * dstImage.rowBytes) + x * numChannels;
                dPixel[0] = static_cast<uint8_t>(Math::clamp(c.r * 255.0f, 0.0f, 255.0f));
                dPixel[1] = static_cast<uint8_t>(Math::clamp(c.g * 255.0f, 0.0f, 255.0f));
                dPixel[2] = static_cast<uint8_t>(Math::clamp(c.b * 255.0f, 0.0f, 255.0f));
                if (numChannels == 4) dPixel[3] = static_cast<uint8_t>(Math::clamp(c.a * 255.0f, 0.0f, 255.0f));
            }
        }
    });
}

} // namespace AIDepthPro
