#include "TemporalStabilizer.h"
#include "../Core/ThreadPool.h"
#include "../Core/MathUtils.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace AIDepthPro {

TemporalStabilizer::TemporalStabilizer() = default;
TemporalStabilizer::~TemporalStabilizer() = default;

void TemporalStabilizer::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hasHistory = false;
    m_prevImageBytes.clear();
}

void TemporalStabilizer::Stabilize(
    const ImageFrame& currImage,
    const DepthFrame& currDepth,
    DepthFrame& stableDepth,
    float temporalStability,
    bool useMotionCompensation,
    float flickerReduction
) {
    int w = currDepth.width;
    int h = currDepth.height;
    if (w <= 0 || h <= 0) {
        stableDepth = currDepth;
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // If temporal stability is 0 or no previous frame history or resolution changed, bypass and store
    if (temporalStability <= 0.001f || !m_hasHistory || m_prevDepth.width != w || m_prevDepth.height != h) {
        stableDepth = currDepth;
        m_prevDepth = currDepth;
        m_prevMeanDepth = 0.0f;
        for (float value : currDepth.data) m_prevMeanDepth += value;
        m_prevMeanDepth /= (w * h);

        // Store copy of current image for optical flow
        const size_t packedStride = size_t(currImage.width) * (currImage.components == PixelComponent::RGBA ? 4 : 3) *
            (currImage.bitDepth == BitDepth::Float ? sizeof(float) : 1);
        size_t imgSize = packedStride * currImage.height;
        if (imgSize > 0) {
            m_prevImageBytes.resize(imgSize);
            for (int row=0;row<currImage.height;++row)
                std::memcpy(m_prevImageBytes.data()+row*packedStride,
                    static_cast<const char*>(currImage.data)+row*currImage.rowBytes,packedStride);
            m_prevImage = currImage;
            m_prevImage.rowBytes = packedStride;
            m_prevImage.data = m_prevImageBytes.data();
        }

        m_hasHistory = true;
        return;
    }

    stableDepth.resize(w, h);

    // Estimate motion if motion compensation is enabled
    if (useMotionCompensation && m_prevImage.isValid()) {
        MotionEstimator::EstimateMotion(m_prevImage, currImage, m_motionField);
    }

    // Compute current frame depth mean for flicker reduction
    float currMeanDepth = 0.0f;
    for (int i = 0; i < w * h; ++i) {
        currMeanDepth += currDepth.data[i];
    }
    currMeanDepth /= (w * h);

    float meanDiff = currMeanDepth - m_prevMeanDepth;
    float flickerCorrection = meanDiff * Math::clamp(flickerReduction, 0.0f, 1.0f);

    float baseAlpha = Math::clamp(temporalStability, 0.0f, 0.95f);

    ThreadPool::getInstance().parallelFor(0, h, [&](int y) {
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;
            float cDepth = currDepth.data[idx] - flickerCorrection;
            cDepth = Math::clamp(cDepth, 0.0f, 1.0f);

            float pDepth = 0.0f;
            float motionMag = 0.0f;
            float confidence = 1.0f;

            if (useMotionCompensation && m_motionField.width == w && m_motionField.height == h) {
                float vx = m_motionField.vx[idx];
                float vy = m_motionField.vy[idx];
                confidence = m_motionField.confidence[idx];
                motionMag = std::sqrt(vx * vx + vy * vy);

                // Sample previous depth using motion vector back-projection
                float prevX = x + vx;
                float prevY = y + vy;
                float u = Math::clamp(prevX / (w > 1 ? (w - 1) : 1), 0.0f, 1.0f);
                float v = Math::clamp(prevY / (h > 1 ? (h - 1) : 1), 0.0f, 1.0f);
                pDepth = m_prevDepth.sampleBilinear(u, v);
            } else {
                pDepth = m_prevDepth.data[idx];
            }

            // Adaptive blending factor: decay alpha when motion is high to eliminate ghosting
            float motionDecay = std::exp(-motionMag * 0.5f) * confidence;
            float alpha = baseAlpha * motionDecay;

            float depthDiff = std::abs(cDepth - pDepth);
            if (depthDiff > 0.4f) {
                // Large structural depth edge appearance - prioritize current frame immediately
                alpha *= (1.0f - (depthDiff - 0.4f) / 0.6f);
            }

            float sDepth = (1.0f - alpha) * cDepth + alpha * pDepth;
            stableDepth.data[idx] = Math::clamp(sDepth, 0.0f, 1.0f);
        }
    });

    // Update history
    m_prevDepth = stableDepth;
    m_prevMeanDepth = currMeanDepth;

    const size_t packedStride = size_t(currImage.width) * (currImage.components == PixelComponent::RGBA ? 4 : 3) *
            (currImage.bitDepth == BitDepth::Float ? sizeof(float) : 1);
        size_t imgSize = packedStride * currImage.height;
    if (imgSize > 0) {
        m_prevImageBytes.resize(imgSize);
        for (int row=0;row<currImage.height;++row)
                std::memcpy(m_prevImageBytes.data()+row*packedStride,
                    static_cast<const char*>(currImage.data)+row*currImage.rowBytes,packedStride);
        m_prevImage = currImage;
            m_prevImage.rowBytes = packedStride;
        m_prevImage.data = m_prevImageBytes.data();
    }
}

} // namespace AIDepthPro
