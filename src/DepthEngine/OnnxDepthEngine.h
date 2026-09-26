#pragma once

#include "DepthEngine.h"
#include <vector>
#include <string>
#include <memory>

namespace AIDepthPro {

class OnnxDepthEngine : public DepthEngine {
public:
    explicit OnnxDepthEngine(EngineType type = EngineType::CPU);
    virtual ~OnnxDepthEngine() override;

    virtual bool Initialize(const EngineConfig& config) override;
    virtual bool GenerateDepth(const ImageFrame& input, DepthFrame& outputDepth) override;
    virtual void Shutdown() override;
    virtual bool IsReady() const override;
    virtual EngineStats GetStats() const override;
    virtual const char* GetBackendName() const override;

private:
    EngineType m_type;
    EngineConfig m_config;
    EngineStats m_stats;
    bool m_isInitialized = false;

    int m_inputW = 518;
    int m_inputH = 518;

    std::vector<float> m_inputTensorData;
    std::vector<float> m_outputTensorData;

    // Internal PIMPL for ONNX Runtime session & environment handles
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace AIDepthPro
