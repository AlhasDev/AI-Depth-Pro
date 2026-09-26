#include "../src/Core/Types.h"
#include "../src/Cache/LRUCache.h"
#include "../src/Temporal/MotionEstimator.h"
#include "../src/Temporal/TemporalStabilizer.h"
#include "../src/Processing/EdgeRefinement.h"
#include <iostream>
#include <vector>
#include <cmath>

extern void reportTest(const std::string& name, bool passed, const std::string& details = "");

void runTemporalAndCacheTests() {
    using namespace AIDepthPro;

    // 1. LRUCache Tests
    {
        LRUCache cache(3, 10); // max 3 frames, 10 MB

        CacheKey k1{ 0.0, 1920, 1080, QualityMode::Balanced, 123, "model.onnx" };
        CacheKey k2{ 1.0, 1920, 1080, QualityMode::Balanced, 123, "model.onnx" };
        CacheKey k3{ 2.0, 1920, 1080, QualityMode::Balanced, 123, "model.onnx" };
        CacheKey k4{ 3.0, 1920, 1080, QualityMode::Balanced, 123, "model.onnx" };

        DepthFrame f(100, 100);
        f.set(10, 10, 0.75f);

        cache.put(k1, f);
        cache.put(k2, f);
        cache.put(k3, f);

        DepthFrame retrieved;
        bool hit1 = cache.get(k1, retrieved);
        reportTest("LRUCache Hit on Stored Frame", hit1 && retrieved.get(10, 10) == 0.75f);

        // Put 4th frame -> should evict k2 (since k1 was accessed recently)
        cache.put(k4, f);
        bool hit2 = cache.get(k2, retrieved);
        reportTest("LRUCache Eviction of Least Recently Used Frame", !hit2);
        reportTest("LRUCache Hit Rate Tracking", cache.getHitRate() > 0.0f);
    }

    // 2. Motion Estimator Test
    {
        int w = 128, h = 128;
        std::vector<float> img1(w * h * 4, 0.2f);
        std::vector<float> img2(w * h * 4, 0.2f);

        // Draw a bright square in img1 at (40, 40)
        for (int y = 40; y < 60; ++y) {
            for (int x = 40; x < 60; ++x) {
                img1[(y * w + x) * 4] = 1.0f;
            }
        }
        // Draw the square shifted in img2 to (42, 40) -> dx = +2
        for (int y = 40; y < 60; ++y) {
            for (int x = 42; x < 62; ++x) {
                img2[(y * w + x) * 4] = 1.0f;
            }
        }

        ImageFrame f1{ img1.data(), w, h, static_cast<size_t>(w * 4 * sizeof(float)), BitDepth::Float, PixelComponent::RGBA };
        ImageFrame f2{ img2.data(), w, h, static_cast<size_t>(w * 4 * sizeof(float)), BitDepth::Float, PixelComponent::RGBA };

        MotionField mf;
        MotionEstimator::EstimateMotion(f1, f2, mf, 8);

        bool validSize = (mf.width == w && mf.height == h);
        reportTest("MotionEstimator Field Computation", validSize);
    }

    // 3. Temporal Stabilizer Test
    {
        int w = 64, h = 64;
        std::vector<float> imgData(w * h * 4, 0.5f);
        ImageFrame frame{ imgData.data(), w, h, static_cast<size_t>(w * 4 * sizeof(float)), BitDepth::Float, PixelComponent::RGBA };

        DepthFrame d1(w, h);
        DepthFrame d2(w, h);
        for (int i = 0; i < w * h; ++i) {
            d1.data[i] = 0.5f;
            d2.data[i] = 0.6f; // slight flicker jump
        }

        TemporalStabilizer stabilizer;
        DepthFrame s1, s2;

        stabilizer.Stabilize(frame, d1, s1, 0.5f, true, 0.5f);
        stabilizer.Stabilize(frame, d2, s2, 0.5f, true, 0.5f);

        // Frame 2 smoothed should be between 0.5 and 0.6
        float smoothedVal = s2.get(w / 2, h / 2);
        bool isSmoothed = (smoothedVal > 0.50f && smoothedVal < 0.60f);

        reportTest("TemporalStabilizer Flicker & Jitter Smoothing", isSmoothed);
    }

    // 4. Guided Filter Edge Refinement Test
    {
        int w = 128, h = 128;
        std::vector<float> rgbData(w * h * 4, 0.0f);
        // Sharp step edge in RGB guide at x = 64
        for (int y = 0; y < h; ++y) {
            for (int x = 64; x < w; ++x) {
                rgbData[(y * w + x) * 4 + 0] = 1.0f;
                rgbData[(y * w + x) * 4 + 1] = 1.0f;
                rgbData[(y * w + x) * 4 + 2] = 1.0f;
                rgbData[(y * w + x) * 4 + 3] = 1.0f;
            }
        }
        ImageFrame guide{ rgbData.data(), w, h, static_cast<size_t>(w * 4 * sizeof(float)), BitDepth::Float, PixelComponent::RGBA };

        // Blurry raw depth step around x = 64
        DepthFrame rawD(w, h);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                float v = (x < 60) ? 0.2f : ((x > 68) ? 0.8f : 0.5f);
                rawD.set(x, y, v);
            }
        }

        DepthFrame refinedD;
        EdgeRefinement::ApplyGuidedFilter(guide, rawD, refinedD, 4, 0.001f);

        float leftVal = refinedD.get(55, h / 2);
        float rightVal = refinedD.get(75, h / 2);

        reportTest("GuidedFilter Edge-Preserving Sharpening", (leftVal < 0.35f && rightVal > 0.65f));
    }
}
