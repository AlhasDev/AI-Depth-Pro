#include "OnnxDepthEngine.h"
#include "../Utils/FileUtils.h"
#include "../Utils/Logger.h"
#include "../../include/onnxruntime_c_api.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace AIDepthPro {
namespace {
void check(const OrtApi* api, OrtStatus* status) {
    if (!status) return;
    std::string error = api->GetErrorMessage(status);
    api->ReleaseStatus(status);
    throw std::runtime_error(error);
}
}
struct OnnxDepthEngine::Impl {
    void* library = nullptr;
    const OrtApi* api = nullptr;
    OrtEnv* env = nullptr;
    OrtSession* session = nullptr;
    OrtMemoryInfo* memory = nullptr;
    OrtValue* inputTensor = nullptr;
    std::string inputName, outputName;
};
OnnxDepthEngine::OnnxDepthEngine(EngineType type) : m_type(type), m_impl(std::make_unique<Impl>()) {}
OnnxDepthEngine::~OnnxDepthEngine() { Shutdown(); }

bool OnnxDepthEngine::Initialize(const EngineConfig& config) {
    Shutdown();
    m_config = config;
    m_stats = EngineStats{};
    try {
        if (m_type != EngineType::Auto && m_type != EngineType::CPU && m_type != EngineType::DirectML)
            throw std::runtime_error("Requested backend is not implemented");
        const auto model = FileUtils::findModelFile(config.modelPath.empty() ? "depth_anything_v2_vits.onnx" : config.modelPath);
        if (model.empty()) throw std::runtime_error("ONNX model not found: " + config.modelPath);
#if defined(_WIN32)
        const char* libraryName = "onnxruntime.dll";
#elif defined(__APPLE__)
        const char* libraryName = "libonnxruntime.dylib";
#else
        const char* libraryName = "libonnxruntime.so";
#endif
        std::vector<std::string> dirs;
        if (const char* dir = std::getenv("AI_DEPTH_PRO_RUNTIME_DIR")) dirs.emplace_back(dir);
        dirs.push_back(FileUtils::getPluginDirectory());
        for (const auto& dir : dirs) {
            if (dir.empty()) continue;
            const auto path = std::filesystem::absolute(std::filesystem::path(dir) / libraryName);
#if defined(_WIN32)
            m_impl->library = LoadLibraryExW(path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
#else
            m_impl->library = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
            if (m_impl->library) break;
        }
        if (!m_impl->library) throw std::runtime_error("ONNX Runtime missing; place it beside the plugin or set AI_DEPTH_PRO_RUNTIME_DIR");
        using GetApi = const OrtApiBase* (ORT_API_CALL*)(void);
#if defined(_WIN32)
        auto getApi = reinterpret_cast<GetApi>(GetProcAddress(static_cast<HMODULE>(m_impl->library), "OrtGetApiBase"));
#else
        auto getApi = reinterpret_cast<GetApi>(dlsym(m_impl->library, "OrtGetApiBase"));
#endif
        if (!getApi || !(m_impl->api = getApi()->GetApi(ORT_API_VERSION)))
            throw std::runtime_error("Incompatible ONNX Runtime API version");
        const auto* api = m_impl->api;
        check(api, api->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "AIDepthPro", &m_impl->env));
        auto createSession = [&](bool gpu) {
            OrtSessionOptions* rawOptions = nullptr;
            check(api, api->CreateSessionOptions(&rawOptions));
            auto options = std::unique_ptr<OrtSessionOptions, decltype(api->ReleaseSessionOptions)>(rawOptions, api->ReleaseSessionOptions);
            check(api, api->SetIntraOpNumThreads(options.get(), std::max(1, config.numThreads)));
            check(api, api->SetSessionGraphOptimizationLevel(options.get(), ORT_ENABLE_ALL));
            check(api, api->SetSessionExecutionMode(options.get(), ORT_SEQUENTIAL));
            if (gpu) {
#if defined(_WIN32)
                using AppendDml = OrtStatus* (ORT_API_CALL*)(OrtSessionOptions*, int);
                auto append = reinterpret_cast<AppendDml>(GetProcAddress(static_cast<HMODULE>(m_impl->library), "OrtSessionOptionsAppendExecutionProvider_DML"));
                if (!append) throw std::runtime_error("This ONNX Runtime build has no DirectML provider");
                check(api, api->DisableMemPattern(options.get()));
                check(api, append(options.get(), config.preferredDeviceIndex));
#else
                throw std::runtime_error("DirectML requires Windows");
#endif
            }
            const auto modelPath = std::filesystem::absolute(model);
            check(api, api->CreateSession(m_impl->env, modelPath.c_str(), options.get(), &m_impl->session));
            m_stats.isGpuAccelerated = gpu;
            m_stats.backendName = gpu ? "ONNX / DirectML" : "ONNX / CPU";
        };
        bool preferGpu = m_type == EngineType::DirectML;
#if defined(_WIN32)
        preferGpu = preferGpu || m_type == EngineType::Auto;
#endif
        try { createSession(preferGpu); }
        catch (const std::exception& e) {
            if (m_type != EngineType::Auto || !preferGpu) throw;
            LOG_WARN(std::string("DirectML unavailable; using real ONNX CPU inference: ") + e.what());
            if (m_impl->session) { api->ReleaseSession(m_impl->session); m_impl->session = nullptr; }
            createSession(false);
        }
        size_t count = 0;
        check(api, api->SessionGetInputCount(m_impl->session, &count));
        if (count != 1) throw std::runtime_error("Expected a single RGB input tensor");
        check(api, api->SessionGetOutputCount(m_impl->session, &count));
        if (count != 1) throw std::runtime_error("Expected a single depth output tensor");
        OrtAllocator* allocator = nullptr;
        check(api, api->GetAllocatorWithDefaultOptions(&allocator));
        char* name = nullptr;
        check(api, api->SessionGetInputName(m_impl->session, 0, allocator, &name));
        m_impl->inputName = name; allocator->Free(allocator, name);
        check(api, api->SessionGetOutputName(m_impl->session, 0, allocator, &name));
        m_impl->outputName = name; allocator->Free(allocator, name);
        OrtTypeInfo* rawInfo = nullptr;
        check(api, api->SessionGetInputTypeInfo(m_impl->session, 0, &rawInfo));
        auto info = std::unique_ptr<OrtTypeInfo, decltype(api->ReleaseTypeInfo)>(rawInfo, api->ReleaseTypeInfo);
        const OrtTensorTypeAndShapeInfo* shapeInfo = nullptr;
        check(api, api->CastTypeInfoToTensorInfo(info.get(), &shapeInfo));
        if (!shapeInfo) throw std::runtime_error("Input must be a tensor");
        ONNXTensorElementDataType type;
        check(api, api->GetTensorElementType(shapeInfo, &type));
        check(api, api->GetDimensionsCount(shapeInfo, &count));
        if (type != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT || count != 4)
            throw std::runtime_error("Expected float32 NCHW input");
        int64_t dims[4];
        check(api, api->GetDimensions(shapeInfo, dims, 4));
        if ((dims[0] > 0 && dims[0] != 1) || (dims[1] > 0 && dims[1] != 3))
            throw std::runtime_error("Expected batch 1 and three input channels");
        int resolution = config.qualityMode == QualityMode::Fast ? 280 : config.qualityMode == QualityMode::Balanced ? 392 : 518;
        if (dims[2] > 4096 || dims[3] > 4096) throw std::runtime_error("Unsupported model resolution");
        m_inputH = dims[2] > 0 ? static_cast<int>(dims[2]) : resolution;
        m_inputW = dims[3] > 0 ? static_cast<int>(dims[3]) : resolution;
        m_inputTensorData.resize(size_t(3) * m_inputW * m_inputH);
        int64_t inputShape[] = {1, 3, m_inputH, m_inputW};
        check(api, api->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &m_impl->memory));
        check(api, api->CreateTensorWithDataAsOrtValue(m_impl->memory, m_inputTensorData.data(), m_inputTensorData.size() * sizeof(float), inputShape, 4, ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &m_impl->inputTensor));
        m_stats.modelName = model;
        m_isInitialized = true;
        LOG_INFO("Real depth model ready: " + m_stats.backendName + " / " + model);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Depth initialization failed: ") + e.what());
        Shutdown();
        return false;
    }
}

bool OnnxDepthEngine::GenerateDepth(const ImageFrame& input, DepthFrame& outputDepth) {
    outputDepth = DepthFrame{};
    if (!m_isInitialized || !input.isValid() || (input.bitDepth != BitDepth::Float && input.bitDepth != BitDepth::Byte) || input.components == PixelComponent::Alpha) return false;
    try {
        const auto start = std::chrono::steady_clock::now();
        int validW = 0, validH = 0, padX = 0, padY = 0;
        PreprocessImage(input, m_inputW, m_inputH, m_inputTensorData, validW, validH, padX, padY);
        const auto pre = std::chrono::steady_clock::now();
        const auto* api = m_impl->api;
        const char* inputNames[] = {m_impl->inputName.c_str()};
        const char* outputNames[] = {m_impl->outputName.c_str()};
        const OrtValue* inputs[] = {m_impl->inputTensor};
        OrtValue* rawOutput = nullptr;
        OrtStatus* runStatus = api->Run(m_impl->session, nullptr, inputNames, inputs, 1, outputNames, 1, &rawOutput);
        auto output = std::unique_ptr<OrtValue, decltype(api->ReleaseValue)>(rawOutput, api->ReleaseValue);
        check(api, runStatus);
        if (!output) throw std::runtime_error("Model produced no output");
        const auto infer = std::chrono::steady_clock::now();
        OrtTensorTypeAndShapeInfo* rawShape = nullptr;
        check(api, api->GetTensorTypeAndShape(output.get(), &rawShape));
        auto shape = std::unique_ptr<OrtTensorTypeAndShapeInfo, decltype(api->ReleaseTensorTypeAndShapeInfo)>(rawShape, api->ReleaseTensorTypeAndShapeInfo);
        size_t rank = 0;
        ONNXTensorElementDataType type;
        check(api, api->GetDimensionsCount(shape.get(), &rank));
        check(api, api->GetTensorElementType(shape.get(), &type));
        if (type != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT || rank < 2 || rank > 4)
            throw std::runtime_error("Expected a float32 depth map");
        std::vector<int64_t> dims(rank);
        check(api, api->GetDimensions(shape.get(), dims.data(), rank));
        for (size_t i = 0; i + 2 < rank; ++i) if (dims[i] != 1) throw std::runtime_error("Expected single-channel depth");
        if (dims[rank-2] < 1 || dims[rank-1] < 1 || dims[rank-2] > 8192 || dims[rank-1] > 8192) throw std::runtime_error("Invalid depth output dimensions");
        const int h = static_cast<int>(dims[rank - 2]), w = static_cast<int>(dims[rank - 1]);
        float* depth = nullptr;
        check(api, api->GetTensorMutableData(output.get(), reinterpret_cast<void**>(&depth)));
        for (size_t i = 0; i < size_t(w) * h; ++i) if (!std::isfinite(depth[i])) throw std::runtime_error("Non-finite model output");
        const int px = std::clamp(int(std::round(double(padX) * w / m_inputW)), 0, w - 1);
        const int py = std::clamp(int(std::round(double(padY) * h / m_inputH)), 0, h - 1);
        const int vw = std::clamp(int(std::round(double(validW) * w / m_inputW)), 1, w - px);
        const int vh = std::clamp(int(std::round(double(validH) * h / m_inputH)), 1, h - py);
        PostprocessDepth(depth, w, h, vw, vh, px, py, input.width, input.height, outputDepth);
        const auto end = std::chrono::steady_clock::now();
        m_stats.inferenceTimeMs = std::chrono::duration<float, std::milli>(infer - pre).count();
        m_stats.postProcessTimeMs = std::chrono::duration<float, std::milli>(end - infer).count();
        m_stats.totalFrameTimeMs = std::chrono::duration<float, std::milli>(end - start).count();
        m_stats.currentFps = 1000.0f / std::max(0.001f, m_stats.totalFrameTimeMs);
        m_stats.memoryUsedBytes = m_inputTensorData.size() * sizeof(float);
        return outputDepth.isValid();
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Depth inference failed: ") + e.what());
        outputDepth = DepthFrame{};
        return false;
    }
}
void OnnxDepthEngine::Shutdown() {
    m_isInitialized = false;
    if (!m_impl) return;
    const auto* api = m_impl->api;
    if (api) {
        if (m_impl->inputTensor) api->ReleaseValue(m_impl->inputTensor);
        if (m_impl->memory) api->ReleaseMemoryInfo(m_impl->memory);
        if (m_impl->session) api->ReleaseSession(m_impl->session);
        if (m_impl->env) api->ReleaseEnv(m_impl->env);
    }
    if (m_impl->library) {
#if defined(_WIN32)
        FreeLibrary(static_cast<HMODULE>(m_impl->library));
#else
        dlclose(m_impl->library);
#endif
    }
    *m_impl = Impl{};
}
bool OnnxDepthEngine::IsReady() const { return m_isInitialized; }
EngineStats OnnxDepthEngine::GetStats() const { return m_stats; }
const char* OnnxDepthEngine::GetBackendName() const { return m_stats.backendName.c_str(); }
} // namespace AIDepthPro
