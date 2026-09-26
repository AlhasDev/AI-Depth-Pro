#include "EdgeRefinement.h"
#include "../Core/ThreadPool.h"
#include "../Core/MathUtils.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace AIDepthPro {

namespace {

// Sliding sums eliminate thousands of row/column allocations. Vertical work is
// tiled across adjacent columns so each worker reads contiguous memory.
void boxFilter(const std::vector<float>& src, std::vector<float>& dst, int width, int height, int r) {
    dst.resize(size_t(width)*height);
    std::vector<float> temp(size_t(width)*height);
    ThreadPool::getInstance().parallelFor(0,height,[&](int y) {
        const float* row=src.data()+size_t(y)*width;
        float* target=temp.data()+size_t(y)*width;
        double sum=0;
        for(int x=0;x<=std::min(r,width-1);++x) sum+=row[x];
        for(int x=0;x<width;++x) {
            const int count=std::min(width-1,x+r)-std::max(0,x-r)+1;
            target[x]=static_cast<float>(sum/count);
            if(x-r>=0) sum-=row[x-r];
            if(x+r+1<width) sum+=row[x+r+1];
        }
    });
    constexpr int tile=64;
    ThreadPool::getInstance().parallelFor(0,(width+tile-1)/tile,[&](int block) {
        const int first=block*tile, count=std::min(tile,width-first);
        double sums[64]{};
        for(int y=0;y<=std::min(r,height-1);++y)
            for(int i=0;i<count;++i) sums[i]+=temp[size_t(y)*width+first+i];
        for(int y=0;y<height;++y) {
            const double divisor=std::min(height-1,y+r)-std::max(0,y-r)+1;
            auto* target=dst.data()+size_t(y)*width+first;
            for(int i=0;i<count;++i) target[i]=static_cast<float>(sums[i]/divisor);
            if(y-r>=0) {
                const auto* row=temp.data()+size_t(y-r)*width+first;
                for(int i=0;i<count;++i) sums[i]-=row[i];
            }
            if(y+r+1<height) {
                const auto* row=temp.data()+size_t(y+r+1)*width+first;
                for(int i=0;i<count;++i) sums[i]+=row[i];
            }
        }
    },1);
}
} // anonymous namespace

void EdgeRefinement::ApplyGuidedFilter(
    const ImageFrame& rgbGuide,
    const DepthFrame& rawDepth,
    DepthFrame& refinedDepth,
    int radius,
    float eps
) {
    int w = rawDepth.width;
    int h = rawDepth.height;
    if (w <= 0 || h <= 0 || !rgbGuide.isValid()) {
        refinedDepth = rawDepth;
        return;
    }

    refinedDepth.resize(w, h);
    radius = Math::clamp(radius, 1, 16);
    eps = std::max(eps, 1e-6f);

    // 1. Extract guide image luminance
    std::vector<float> I(w * h);
    const float* floatGuide = static_cast<const float*>(rgbGuide.data);
    const uint8_t* byteGuide = static_cast<const uint8_t*>(rgbGuide.data);
    int numChannels = (rgbGuide.components == PixelComponent::RGBA) ? 4 : 3;

    ThreadPool::getInstance().parallelFor(0, h, [&](int y) {
        for (int x = 0; x < w; ++x) {
            float r = 0.0f, g = 0.0f, b = 0.0f;
            if (rgbGuide.bitDepth == BitDepth::Float) {
                const float* p = (const float*)((const char*)floatGuide + y * rgbGuide.rowBytes) + x * numChannels;
                r = p[0]; g = p[1]; b = p[2];
            } else {
                const uint8_t* p = (const uint8_t*)((const char*)byteGuide + y * rgbGuide.rowBytes) + x * numChannels;
                r = p[0] / 255.0f; g = p[1] / 255.0f; b = p[2] / 255.0f;
            }
            I[y * w + x] = Math::rgbToLuminance(r, g, b);
        }
    });

    const std::vector<float>& p = rawDepth.data;

    std::vector<float> mean_I, mean_p, mean_Ip, mean_II;
    std::vector<float> Ip(w * h), II(w * h);

    for (int i = 0; i < w * h; ++i) {
        Ip[i] = I[i] * p[i];
        II[i] = I[i] * I[i];
    }

    boxFilter(I, mean_I, w, h, radius);
    boxFilter(p, mean_p, w, h, radius);
    boxFilter(Ip, mean_Ip, w, h, radius);
    boxFilter(II, mean_II, w, h, radius);

    std::vector<float> a(w * h), b(w * h);
    ThreadPool::getInstance().parallelFor(0, h, [&](int y) {
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;
            float cov_Ip = mean_Ip[idx] - mean_I[idx] * mean_p[idx];
            float var_I = mean_II[idx] - mean_I[idx] * mean_I[idx];
            float a_val = cov_Ip / (var_I + eps);
            float b_val = mean_p[idx] - a_val * mean_I[idx];
            a[idx] = a_val;
            b[idx] = b_val;
        }
    });

    std::vector<float> mean_a, mean_b;
    boxFilter(a, mean_a, w, h, radius);
    boxFilter(b, mean_b, w, h, radius);

    ThreadPool::getInstance().parallelFor(0, h, [&](int y) {
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;
            float q = mean_a[idx] * I[idx] + mean_b[idx];
            refinedDepth.set(x, y, Math::clamp(q, 0.0f, 1.0f));
        }
    });
}

void EdgeRefinement::ApplyJointBilateralFilter(
    const ImageFrame& rgbGuide,
    const DepthFrame& rawDepth,
    DepthFrame& refinedDepth,
    int radius,
    float spatialSigma,
    float rangeSigma
) {
    int w = rawDepth.width;
    int h = rawDepth.height;
    if (w <= 0 || h <= 0 || !rgbGuide.isValid()) {
        refinedDepth = rawDepth;
        return;
    }

    refinedDepth.resize(w, h);
    radius = Math::clamp(radius, 1, 8);
    float spatialCoeff = -0.5f / (spatialSigma * spatialSigma);
    float rangeCoeff = -0.5f / (rangeSigma * rangeSigma);

    const float* floatGuide = static_cast<const float*>(rgbGuide.data);
    const uint8_t* byteGuide = static_cast<const uint8_t*>(rgbGuide.data);
    int numChannels = (rgbGuide.components == PixelComponent::RGBA) ? 4 : 3;

    ThreadPool::getInstance().parallelFor(0, h, [&](int y) {
        for (int x = 0; x < w; ++x) {
            float centerLum = 0.0f;
            if (rgbGuide.bitDepth == BitDepth::Float) {
                const float* p = (const float*)((const char*)floatGuide + y * rgbGuide.rowBytes) + x * numChannels;
                centerLum = Math::rgbToLuminance(p[0], p[1], p[2]);
            } else {
                const uint8_t* p = (const uint8_t*)((const char*)byteGuide + y * rgbGuide.rowBytes) + x * numChannels;
                centerLum = Math::rgbToLuminance(p[0] / 255.0f, p[1] / 255.0f, p[2] / 255.0f);
            }

            float weightSum = 0.0f;
            float depthSum = 0.0f;

            for (int dy = -radius; dy <= radius; ++dy) {
                int ny = Math::clamp(y + dy, 0, h - 1);
                for (int dx = -radius; dx <= radius; ++dx) {
                    int nx = Math::clamp(x + dx, 0, w - 1);

                    float neighborLum = 0.0f;
                    if (rgbGuide.bitDepth == BitDepth::Float) {
                        const float* np = (const float*)((const char*)floatGuide + ny * rgbGuide.rowBytes) + nx * numChannels;
                        neighborLum = Math::rgbToLuminance(np[0], np[1], np[2]);
                    } else {
                        const uint8_t* np = (const uint8_t*)((const char*)byteGuide + ny * rgbGuide.rowBytes) + nx * numChannels;
                        neighborLum = Math::rgbToLuminance(np[0] / 255.0f, np[1] / 255.0f, np[2] / 255.0f);
                    }

                    float distSq = static_cast<float>(dx * dx + dy * dy);
                    float lumDiff = centerLum - neighborLum;
                    float lumDiffSq = lumDiff * lumDiff;

                    float wSpatial = std::exp(distSq * spatialCoeff);
                    float wRange = std::exp(lumDiffSq * rangeCoeff);
                    float weight = wSpatial * wRange;

                    depthSum += rawDepth.get(nx, ny) * weight;
                    weightSum += weight;
                }
            }

            refinedDepth.set(x, y, (weightSum > 1e-6f) ? (depthSum / weightSum) : rawDepth.get(x, y));
        }
    });
}

} // namespace AIDepthPro

