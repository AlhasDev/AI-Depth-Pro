#include "../src/Core/Types.h"
#include "../src/DepthEngine/DepthEngine.h"
#include "../src/DepthEngine/OnnxDepthEngine.h"
#include <iostream>
#include <vector>
#include <cmath>

extern void reportTest(const std::string& name, bool passed, const std::string& details = "");

void runDepthEngineTests() {
    using namespace AIDepthPro;

    // Test 1: Factory Creation
    {
        auto engine = DepthEngineFactory::Create(EngineType::CPU);
        reportTest("DepthEngineFactory::Create(CPU)", engine != nullptr);
    }

    // Test 2: Preprocessing and Letterboxing
    {
        int w = 1920, h = 1080;
        std::vector<float> fakeImg(w * h * 4, 0.5f);
        ImageFrame frame;
        frame.data = fakeImg.data();
        frame.width = w;
        frame.height = h;
        frame.rowBytes = w * 4 * sizeof(float);
        frame.bitDepth = BitDepth::Float;
        frame.components = PixelComponent::RGBA;

        std::vector<float> tensorData;
        int validW = 0, validH = 0, padX = 0, padY = 0;
        DepthEngine::PreprocessImage(frame, 518, 518, tensorData, validW, validH, padX, padY);

        bool validBounds = (validW > 0 && validH > 0 && padX >= 0 && padY >= 0);
        bool validSize = (tensorData.size() == 1 * 3 * 518 * 518);
        reportTest("DepthEngine::PreprocessImage (1080p -> 518x518 Letterbox)", validBounds && validSize);
    }

    {
        OnnxDepthEngine engine(EngineType::CPU);
        EngineConfig config;
        config.modelPath = "non_existent_model.onnx";
        reportTest("Missing model must fail without substitution", !engine.Initialize(config) && !engine.IsReady());
        reportTest("Unimplemented TensorRT backend cannot report success", !DepthEngineFactory::Create(EngineType::TensorRT));
        reportTest("Unimplemented Metal backend cannot report success", !DepthEngineFactory::Create(EngineType::CoreML));
    }
}