#include "DepthEngine.h"
#include "OnnxDepthEngine.h"
#include "../Core/MathUtils.h"
#include "../Core/ThreadPool.h"
#include "../Utils/Logger.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace AIDepthPro {

void DepthEngine::PreprocessImage(
    const ImageFrame& input,
    int targetW,
    int targetH,
    std::vector<float>& outTensorData,
    int& validW,
    int& validH,
    int& padX,
    int& padY
) {
    if (!input.isValid() || targetW <= 0 || targetH <= 0) return;

    // Calculate aspect-ratio preserving scaling
    float scaleX = static_cast<float>(targetW) / input.width;
    float scaleY = static_cast<float>(targetH) / input.height;
    float scale = std::min(scaleX, scaleY);

    validW = std::clamp(static_cast<int>(std::round(input.width * scale)), 1, targetW);
    validH = std::clamp(static_cast<int>(std::round(input.height * scale)), 1, targetH);

    padX = (targetW - validW) / 2;
    padY = (targetH - validH) / 2;

    outTensorData.resize(size_t(3) * targetH * targetW);
    std::fill(outTensorData.begin(), outTensorData.end(), 0.0f);

    const float mean[3] = { 0.485f, 0.456f, 0.406f };
    const float stdDev[3] = { 0.229f, 0.224f, 0.225f };

    float* rChannel = outTensorData.data() + 0 * targetH * targetW;
    float* gChannel = outTensorData.data() + 1 * targetH * targetW;
    float* bChannel = outTensorData.data() + 2 * targetH * targetW;

    const uint8_t* byteSrc = static_cast<const uint8_t*>(input.data);
    const float* floatSrc = static_cast<const float*>(input.data);
    int numChannels = (input.components == PixelComponent::RGBA) ? 4 : 3;

    ThreadPool::getInstance().parallelFor(0, validH, [&](int vy) {
        int dstY = padY + vy;
        float srcNormY = static_cast<float>(vy) / (validH > 1 ? (validH - 1) : 1);
        float srcY = srcNormY * (input.height - 1);
        int y0 = static_cast<int>(srcY);
        int y1 = std::min(y0 + 1, input.height - 1);
        float fy = srcY - y0;

        for (int vx = 0; vx < validW; ++vx) {
            int dstX = padX + vx;
            float srcNormX = static_cast<float>(vx) / (validW > 1 ? (validW - 1) : 1);
            float srcX = srcNormX * (input.width - 1);
            int x0 = static_cast<int>(srcX);
            int x1 = std::min(x0 + 1, input.width - 1);
            float fx = srcX - x0;

            float r = 0.0f, g = 0.0f, b = 0.0f;

            if (input.bitDepth == BitDepth::Float) {
                const float* p00 = (const float*)((const char*)floatSrc + y0 * input.rowBytes) + x0 * numChannels;
                const float* p10 = (const float*)((const char*)floatSrc + y0 * input.rowBytes) + x1 * numChannels;
                const float* p01 = (const float*)((const char*)floatSrc + y1 * input.rowBytes) + x0 * numChannels;
                const float* p11 = (const float*)((const char*)floatSrc + y1 * input.rowBytes) + x1 * numChannels;

                r = (p00[0] * (1.0f - fx) + p10[0] * fx) * (1.0f - fy) + (p01[0] * (1.0f - fx) + p11[0] * fx) * fy;
                g = (p00[1] * (1.0f - fx) + p10[1] * fx) * (1.0f - fy) + (p01[1] * (1.0f - fx) + p11[1] * fx) * fy;
                b = (p00[2] * (1.0f - fx) + p10[2] * fx) * (1.0f - fy) + (p01[2] * (1.0f - fx) + p11[2] * fx) * fy;
            } else {
                const uint8_t* p00 = (const uint8_t*)((const char*)byteSrc + y0 * input.rowBytes) + x0 * numChannels;
                const uint8_t* p10 = (const uint8_t*)((const char*)byteSrc + y0 * input.rowBytes) + x1 * numChannels;
                const uint8_t* p01 = (const uint8_t*)((const char*)byteSrc + y1 * input.rowBytes) + x0 * numChannels;
                const uint8_t* p11 = (const uint8_t*)((const char*)byteSrc + y1 * input.rowBytes) + x1 * numChannels;

                float r00 = p00[0] / 255.0f, g00 = p00[1] / 255.0f, b00 = p00[2] / 255.0f;
                float r10 = p10[0] / 255.0f, g10 = p10[1] / 255.0f, b10 = p10[2] / 255.0f;
                float r01 = p01[0] / 255.0f, g01 = p01[1] / 255.0f, b01 = p01[2] / 255.0f;
                float r11 = p11[0] / 255.0f, g11 = p11[1] / 255.0f, b11 = p11[2] / 255.0f;

                r = (r00 * (1.0f - fx) + r10 * fx) * (1.0f - fy) + (r01 * (1.0f - fx) + r11 * fx) * fy;
                g = (g00 * (1.0f - fx) + g10 * fx) * (1.0f - fy) + (g01 * (1.0f - fx) + g11 * fx) * fy;
                b = (b00 * (1.0f - fx) + b10 * fx) * (1.0f - fy) + (b01 * (1.0f - fx) + b11 * fx) * fy;
            }

            int dstIdx = dstY * targetW + dstX;
            rChannel[dstIdx] = (r - mean[0]) / stdDev[0];
            gChannel[dstIdx] = (g - mean[1]) / stdDev[1];
            bChannel[dstIdx] = (b - mean[2]) / stdDev[2];
        }
    });
}

void DepthEngine::PostprocessDepth(
    const float* rawDepthData,
    int modelW,
    int modelH,
    int validW,
    int validH,
    int padX,
    int padY,
    int origW,
    int origH,
    DepthFrame& outDepth
) {
    if (!rawDepthData || modelW <= 0 || modelH <= 0 || origW <= 0 || origH <= 0) return;

    outDepth.resize(origW, origH);

    // Compute min and max within the valid letterboxed bounding box for normalization
    float minVal = 1e9f;
    float maxVal = -1e9f;

    for (int y = padY; y < padY + validH; ++y) {
        for (int x = padX; x < padX + validW; ++x) {
            float v = rawDepthData[y * modelW + x];
            if (!std::isnan(v) && !std::isinf(v)) {
                if (v < minVal) minVal = v;
                if (v > maxVal) maxVal = v;
            }
        }
    }

    float range = maxVal - minVal;
    if (range < 1e-6f) range = 1.0f;

    // Rescale unpadded valid depth to original target resolution
    ThreadPool::getInstance().parallelFor(0, origH, [&](int y) {
        float normY = static_cast<float>(y) / (origH > 1 ? (origH - 1) : 1);
        float sampleY = padY + normY * (validH - 1);
        int sy0 = std::clamp(static_cast<int>(sampleY), 0, modelH - 1);
        int sy1 = std::clamp(sy0 + 1, 0, modelH - 1);
        float fy = sampleY - sy0;

        for (int x = 0; x < origW; ++x) {
            float normX = static_cast<float>(x) / (origW > 1 ? (origW - 1) : 1);
            float sampleX = padX + normX * (validW - 1);
            int sx0 = std::clamp(static_cast<int>(sampleX), 0, modelW - 1);
            int sx1 = std::clamp(sx0 + 1, 0, modelW - 1);
            float fx = sampleX - sx0;

            float d00 = rawDepthData[sy0 * modelW + sx0];
            float d10 = rawDepthData[sy0 * modelW + sx1];
            float d01 = rawDepthData[sy1 * modelW + sx0];
            float d11 = rawDepthData[sy1 * modelW + sx1];

            float rawD = (d00 * (1.0f - fx) + d10 * fx) * (1.0f - fy) + (d01 * (1.0f - fx) + d11 * fx) * fy;
            
            // Normalize to [0.0 = Far, 1.0 = Near]
            float normD = Math::clamp((rawD - minVal) / range, 0.0f, 1.0f);
            outDepth.set(x, y, normD);
        }
    });
}

std::unique_ptr<DepthEngine> DepthEngineFactory::Create(EngineType type) {
    if (!IsBackendSupported(type)) return nullptr;
    return std::make_unique<OnnxDepthEngine>(type);
}
std::vector<EngineType> DepthEngineFactory::GetAvailableBackends() {
    std::vector<EngineType> result{EngineType::Auto, EngineType::CPU};
#if defined(_WIN32)
    result.push_back(EngineType::DirectML);
#endif
    return result;
}
bool DepthEngineFactory::IsBackendSupported(EngineType type) {
    if (type == EngineType::Auto || type == EngineType::CPU) return true;
#if defined(_WIN32)
    if (type == EngineType::DirectML) return true;
#endif
    return false;
}
} // namespace AIDepthPro
