#pragma once

#include "../Core/Types.h"
#include <string>

namespace AIDepthPro {

class OverlayRenderer {
public:
    // Render focus target reticle at focusPoint
    static void DrawFocusTarget(
        ImageFrame& image,
        const Point2D& focusPoint
    );

    // Render diagnostic performance HUD overlay in the top-left corner
    static void DrawPerformanceOverlay(
        ImageFrame& image,
        const EngineStats& stats,
        float renderTimeMs,
        size_t cacheFrames,
        float cacheHitRate
    );
};

} // namespace AIDepthPro
