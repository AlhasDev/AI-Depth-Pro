#include "MotionEstimator.h"
#include "../Core/ThreadPool.h"
#include "../Core/MathUtils.h"
#include <cmath>
#include <algorithm>

namespace AIDepthPro {

void MotionEstimator::EstimateMotion(
    const ImageFrame& prevFrame,
    const ImageFrame& currFrame,
    MotionField& outMotion,
    int blockSize
) {
    int w = currFrame.width;
    int h = currFrame.height;
    if (w <= 0 || h <= 0 || !prevFrame.isValid() || !currFrame.isValid() || prevFrame.width != w || prevFrame.height != h) {
        outMotion.resize(w, h);
        return;
    }

    outMotion.resize(w, h);
    blockSize = Math::clamp(blockSize, 4, 16);

    // Convert each pixel once instead of recomputing luminance for every search candidate.
    std::vector<float> current(size_t(w)*h), previous(size_t(w)*h);
    auto luminance=[&](const ImageFrame& frame, int x,int y) {
        const int channels=frame.components==PixelComponent::RGBA ? 4 : 3;
        const auto* row=static_cast<const char*>(frame.data)+y*frame.rowBytes;
        if(frame.bitDepth==BitDepth::Float) {
            const auto* p=reinterpret_cast<const float*>(row)+x*channels;
            return Math::rgbToLuminance(p[0],p[1],p[2]);
        }
        const auto* p=reinterpret_cast<const uint8_t*>(row)+x*channels;
        return Math::rgbToLuminance(p[0]/255.0f,p[1]/255.0f,p[2]/255.0f);
    };
    ThreadPool::getInstance().parallelFor(0,h,[&](int y) {
        for(int x=0;x<w;++x) {
            current[y*w+x]=luminance(currFrame,x,y);
            previous[y*w+x]=luminance(prevFrame,x,y);
        }
    });
    int searchRadius = 4;

    ThreadPool::getInstance().parallelFor(0, (h + blockSize - 1) / blockSize, [&](int by) {
        int startY = by * blockSize;
        int endY = std::min(startY + blockSize, h);

        for (int bx = 0; bx < (w + blockSize - 1) / blockSize; ++bx) {
            int startX = bx * blockSize;
            int endX = std::min(startX + blockSize, w);

            float bestSad = 1e9f;
            int bestDx = 0;
            int bestDy = 0;

            // Search local displacement vector (dx, dy) minimizing SAD (Sum of Absolute Differences)
            for (int dy = -searchRadius; dy <= searchRadius; ++dy) {
                for (int dx = -searchRadius; dx <= searchRadius; ++dx) {
                    float sad = 0.0f;
                    int count = 0;

                    for (int y = startY; y < endY; y += 2) {
                        for (int x = startX; x < endX; x += 2) {
                            float currL = current[y*w+x];
                            float prevL = previous[std::clamp(y+dy,0,h-1)*w+std::clamp(x+dx,0,w-1)];
                            sad += std::abs(currL - prevL);
                            count++;
                        }
                    }

                    if (count > 0) sad /= count;
                    if (sad < bestSad || (sad == bestSad && dx*dx+dy*dy < bestDx*bestDx+bestDy*bestDy)) {
                        bestSad = sad;
                        bestDx = dx;
                        bestDy = dy;
                    }
                }
            }

            float conf = Math::clamp(1.0f - bestSad * 2.0f, 0.0f, 1.0f);

            for (int y = startY; y < endY; ++y) {
                for (int x = startX; x < endX; ++x) {
                    int idx = y * w + x;
                    outMotion.vx[idx] = static_cast<float>(bestDx);
                    outMotion.vy[idx] = static_cast<float>(bestDy);
                    outMotion.confidence[idx] = conf;
                }
            }
        }
    });
}

} // namespace AIDepthPro
