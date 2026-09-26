#pragma once

#include "../Core/Types.h"
#include <string>
#include <memory>
#include <vector>

namespace AIDepthPro {

class DepthEngine {
public:
    virtual ~DepthEngine() = default;

    // Initialize backend, load model weights and allocate memory
    virtual bool Initialize(const EngineConfig& config) = 0;

    // Run inference on input image and populate output depth frame
    virtual bool GenerateDepth(const ImageFrame& input, DepthFrame& outputDepth) = 0;

    // Release model and device resources
    virtual void Shutdown() = 0;

    // Check if backend is successfully initialized and ready
    virtual bool IsReady() const = 0;

    // Query engine statistics and diagnostics
    virtual EngineStats GetStats() const = 0;

    // Backend identifier name
    virtual const char* GetBackendName() const = 0;

    // Utility: Preprocess RGB frame into letterboxed NCHW float tensor with ImageNet normalization
    static void PreprocessImage(
        const ImageFrame& input,
        int targetW,
        int targetH,
        std::vector<float>& outTensorData,
        int& validW,
        int& validH,
        int& padX,
        int& padY
    );

    // Utility: Postprocess raw depth tensor (unpadding, normalization to [0..1], edge-aware upsampling)
    static void PostprocessDepth(
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
    );
};

class DepthEngineFactory {
public:
    static std::unique_ptr<DepthEngine> Create(EngineType type = EngineType::Auto);
    static std::vector<EngineType> GetAvailableBackends();
    static bool IsBackendSupported(EngineType type);
};

} // namespace AIDepthPro
