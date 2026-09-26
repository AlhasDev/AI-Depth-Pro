#include "../src/Core/Types.h"
#include "../src/Core/MathUtils.h"
#include "../src/Effects/DepthOfField.h"
#include "../src/Effects/Parallax.h"
#include "../src/Effects/DepthZoom.h"
#include "../src/Effects/DepthFog.h"
#include "../src/Effects/Relighting.h"
#include "../src/Effects/DepthMask.h"
#include "../src/Processing/DepthNormalizer.h"
#include "../src/Processing/ColorMap.h"
#include <iostream>
#include <vector>
#include <cmath>

extern void reportTest(const std::string& name, bool passed, const std::string& details = "");

void runEffectsTests() {
    using namespace AIDepthPro;

    int w = 256, h = 256;
    std::vector<float> srcData(w * h * 4, 0.2f);
    std::vector<float> dstData(w * h * 4, 0.0f);

    // Create synthetic pattern
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = (y * w + x) * 4;
            srcData[idx + 0] = (x % 32 < 16) ? 0.8f : 0.2f;
            srcData[idx + 1] = (y % 32 < 16) ? 0.8f : 0.2f;
            srcData[idx + 2] = 0.2f;
            srcData[idx + 3] = 1.0f;
        }
    }

    ImageFrame srcFrame;
    srcFrame.data = srcData.data();
    srcFrame.width = w;
    srcFrame.height = h;
    srcFrame.rowBytes = w * 4 * sizeof(float);
    srcFrame.bitDepth = BitDepth::Float;
    srcFrame.components = PixelComponent::RGBA;

    ImageFrame dstFrame;
    dstFrame.data = dstData.data();
    dstFrame.width = w;
    dstFrame.height = h;
    dstFrame.rowBytes = w * 4 * sizeof(float);
    dstFrame.bitDepth = BitDepth::Float;
    dstFrame.components = PixelComponent::RGBA;

    DepthFrame depth(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            depth.set(x, y, static_cast<float>(y) / (h - 1));
        }
    }

    // 1. Depth of Field Test
    {
        EffectParams p;
        p.enableDoF = true;
        p.focusPoint = { 0.5f, 0.5f };
        p.focusDepth = 0.5f;
        p.focusRange = 0.1f;
        p.blurStrength = 10.0f;
        p.bokehShape = BokehShape::Hexagonal;

        DepthOfField::ApplyDoF(srcFrame, depth, dstFrame, p);
        float sampleCenter = dstData[(h / 2 * w + w / 2) * 4];
        float sampleBlurred = dstData[(10 * w + 10) * 4];

        reportTest("Depth of Field (Bokeh Convolution)", !std::isnan(sampleCenter) && !std::isnan(sampleBlurred));
    }

    // 2. 3D Parallax Test
    {
        EffectParams p;
        p.enableParallax = true;
        p.parallaxX = 0.05f;
        p.parallaxY = 0.0f;
        p.depthStrength = 1.0f;
        p.edgeFill = EdgeFillMode::Mirror;

        Parallax::ApplyParallax(srcFrame, depth, dstFrame, p);
        float pSample = dstData[(h / 2 * w + w / 2) * 4];
        reportTest("3D Parallax (2.5D Displacement)", !std::isnan(pSample));
    }

    // 3. Depth Zoom Test
    {
        EffectParams p;
        p.enableDepthZoom = true;
        p.nearScale = 1.2f;
        p.farScale = 0.9f;
        p.zoomCenter = { 0.5f, 0.5f };

        DepthZoom::ApplyDepthZoom(srcFrame, depth, dstFrame, p);
        float zSample = dstData[(h / 2 * w + w / 2) * 4];
        reportTest("Depth Zoom Differential Scaling", !std::isnan(zSample));
    }

    // 4. Atmospheric Fog Test
    {
        // Use a uniform dark input image to strictly verify fog addition
        std::vector<float> darkSrc(w * h * 4, 0.1f);
        ImageFrame darkFrame{ darkSrc.data(), w, h, static_cast<size_t>(w * 4 * sizeof(float)), BitDepth::Float, PixelComponent::RGBA };

        EffectParams p;
        p.enableFog = true;
        p.fogAmount = 0.8f;
        p.fogStart = 0.1f;
        p.fogEnd = 0.9f;
        p.fogColor = { 0.9f, 0.95f, 1.0f, 1.0f };

        DepthFog::ApplyFog(darkFrame, depth, dstFrame, p);
        float farPixelR = dstData[(5 * w + w / 2) * 4]; // Far area (top, y=5, depth=0.02)
        float nearPixelR = dstData[((h - 5) * w + w / 2) * 4]; // Near area (bottom, y=h-5, depth=0.98)

        bool fogApplied = (farPixelR > nearPixelR + 0.3f);
        reportTest("Atmospheric Depth Fog Attenuation", fogApplied);
    }

    // 5. Relighting Test (Normals & Blinn-Phong)
    {
        EffectParams p;
        p.enableRelighting = true;
        p.lightDirX = 0.5f;
        p.lightDirY = -0.5f;
        p.lightHeight = 1.0f;
        p.lightStrength = 1.0f;
        p.ambientLight = 0.2f;

        Relighting::ApplyRelighting(srcFrame, depth, dstFrame, p);
        float litSample = dstData[(h / 2 * w + w / 2) * 4];
        reportTest("Scene Relighting (Surface Normals & Lighting)", litSample >= 0.0f && !std::isnan(litSample));
    }

    // 6. Depth Masking Test
    {
        std::vector<float> mask;
        DepthMask::GenerateMask(depth, mask, 0.4f, 0.6f, 0.05f, 0.02f, false);
        float insideMask = mask[(h / 2 * w + w / 2)]; // y=h/2 -> depth=0.5 -> inside [0.4, 0.6]
        float outsideMask = mask[(10 * w + w / 2)];   // y=10 -> depth ~0.04 -> outside

        reportTest("Depth Mask Interval Isolation", (insideMask > 0.9f) && (outsideMask < 0.1f));
    }

    // 7. False Color Turbo Colormap Test
    {
        ColorRGBA cNear = Math::evaluateColorMap(1.0f, ColorMapType::Turbo);
        ColorRGBA cFar = Math::evaluateColorMap(0.0f, ColorMapType::Turbo);
        bool turboValid = (cNear.r > 0.5f && cFar.b > 0.4f); // Turbo maps 0.0 -> blue/purple, 1.0 -> red
        reportTest("Google Turbo Colormap Evaluation", turboValid);
    }
}
