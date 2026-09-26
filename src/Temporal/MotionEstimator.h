#pragma once

#include "../Core/Types.h"
#include <vector>

namespace AIDepthPro {

struct MotionField {
    int width = 0;
    int height = 0;
    std::vector<float> vx;
    std::vector<float> vy;
    std::vector<float> confidence;

    void resize(int w, int h) {
        width = w;
        height = h;
        vx.assign(w * h, 0.0f);
        vy.assign(w * h, 0.0f);
        confidence.assign(w * h, 1.0f);
    }
};

class MotionEstimator {
public:
    // Estimate dense motion vector field between prevFrame and currFrame using block-gradient optical flow
    static void EstimateMotion(
        const ImageFrame& prevFrame,
        const ImageFrame& currFrame,
        MotionField& outMotion,
        int blockSize = 8
    );
};

} // namespace AIDepthPro
