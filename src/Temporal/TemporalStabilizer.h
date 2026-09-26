#pragma once

#include "../Core/Types.h"
#include "MotionEstimator.h"
#include <vector>
#include <memory>
#include <mutex>

namespace AIDepthPro {

class TemporalStabilizer {
public:
    TemporalStabilizer();
    ~TemporalStabilizer();

    // Stabilize current depth frame against temporal history
    void Stabilize(
        const ImageFrame& currImage,
        const DepthFrame& currDepth,
        DepthFrame& stableDepth,
        float temporalStability,
        bool useMotionCompensation,
        float flickerReduction
    );

    // Reset temporal history (e.g. on cut detection or timeline seek)
    void Reset();

private:
    std::mutex m_mutex;
    bool m_hasHistory = false;
    DepthFrame m_prevDepth;
    std::vector<uint8_t> m_prevImageBytes;
    ImageFrame m_prevImage;
    MotionField m_motionField;
    float m_prevMeanDepth = 0.5f;
};

} // namespace AIDepthPro
