#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace AIDepthPro {

enum class EngineType {
    Auto = 0,
    CUDA = 1,
    TensorRT = 2,
    DirectML = 3,
    CoreML = 4,
    CPU = 5
};

enum class QualityMode {
    Fast = 0,       // 256x256 or 384x384, minimal temporal smoothing, highest FPS
    Balanced = 1,   // 384x384 or 518x518, joint bilateral edge refine, balanced FPS
    Quality = 2     // 518x518 or native, full guided filter & temporal stabilization
};

enum class ViewMode {
    Original = 0,
    DepthMap = 1,
    InvertedDepth = 2,
    NearFarMask = 3,
    FalseColor = 4,
    EdgeMap = 5
};

enum class ColorMapType {
    Turbo = 0,
    Magma = 1,
    Inferno = 2,
    CoolWarm = 3
};

enum class BokehShape {
    Disk = 0,
    Hexagonal = 1,
    Gaussian = 2
};

enum class EdgeFillMode {
    Mirror = 0,
    Blur = 1,
    Repeat = 2,
    Inpaint = 3,
    Transparent = 4
};

enum class BitDepth {
    Byte = 0,   // 8-bit unsigned char (0..255)
    Half = 1,   // 16-bit float
    Float = 2   // 32-bit float (0.0..1.0)
};

enum class PixelComponent {
    RGBA = 0,
    RGB = 1,
    Alpha = 2
};

struct RectI {
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;

    int width() const { return x2 - x1; }
    int height() const { return y2 - y1; }
};

struct Point2D {
    float x = 0.5f;
    float y = 0.5f;
};

struct ColorRGBA {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

// Represents an input/output video frame in host or GPU memory
struct ImageFrame {
    void* data = nullptr;
    int width = 0;
    int height = 0;
    size_t rowBytes = 0;
    BitDepth bitDepth = BitDepth::Float;
    PixelComponent components = PixelComponent::RGBA;
    bool isGpuBuffer = false;
    void* gpuStream = nullptr; // CUDA stream or Metal command queue pointer if applicable

    bool isValid() const {
        return data != nullptr && width > 0 && height > 0;
    }
};

// Represents a 32-bit float normalized depth frame [0.0 = Far, 1.0 = Near]
struct DepthFrame {
    std::vector<float> data;
    int width = 0;
    int height = 0;

    DepthFrame() = default;
    DepthFrame(int w, int h) : width(w), height(h), data(w * h, 0.0f) {}

    void resize(int w, int h) {
        width = w;
        height = h;
        data.assign(w * h, 0.0f);
    }

    bool isValid() const {
        return width > 0 && height > 0 && !data.empty();
    }

    float* getPtr() { return data.data(); }
    const float* getPtr() const { return data.data(); }

    float get(int x, int y) const {
        if (x < 0) x = 0;
        if (x >= width) x = width - 1;
        if (y < 0) y = 0;
        if (y >= height) y = height - 1;
        return data[y * width + x];
    }

    void set(int x, int y, float v) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            data[y * width + x] = v;
        }
    }

    float sampleBilinear(float u, float v) const {
        if (width <= 0 || height <= 0) return 0.0f;
        float x = u * (width - 1);
        float y = v * (height - 1);
        int x0 = static_cast<int>(x);
        int y0 = static_cast<int>(y);
        int x1 = (x0 + 1 < width) ? x0 + 1 : x0;
        int y1 = (y0 + 1 < height) ? y0 + 1 : y0;
        float fx = x - x0;
        float fy = y - y0;

        float c00 = data[y0 * width + x0];
        float c10 = data[y0 * width + x1];
        float c01 = data[y1 * width + x0];
        float c11 = data[y1 * width + x1];

        float c0 = c00 * (1.0f - fx) + c10 * fx;
        float c1 = c01 * (1.0f - fx) + c11 * fx;
        return c0 * (1.0f - fy) + c1 * fy;
    }
};

struct EngineConfig {
    EngineType engineType = EngineType::Auto;
    QualityMode qualityMode = QualityMode::Balanced;
    std::string modelPath;
    int preferredDeviceIndex = 0;
    int numThreads = 4;
    int inputResolution = 518; // 256, 384, or 518
};

struct EngineStats {
    std::string backendName = "CPU";
    std::string modelName = "None";
    float inferenceTimeMs = 0.0f;
    float postProcessTimeMs = 0.0f;
    float totalFrameTimeMs = 0.0f;
    float currentFps = 0.0f;
    size_t memoryUsedBytes = 0;
    int cachedFramesCount = 0;
    bool isGpuAccelerated = false;
};

struct EffectParams {
    // General
    EngineType engine = EngineType::Auto;
    QualityMode quality = QualityMode::Balanced;
    bool autoMode = true;
    std::string customModelPath;

    // Temporal
    float temporalStability = 0.5f; // 0.0 to 1.0
    bool motionCompensation = true;
    float flickerReduction = 0.3f;

    // Depth Adjustments
    bool invertDepth = false;
    float nearRange = 0.0f;
    float farRange = 1.0f;
    float depthGamma = 1.0f;
    float depthContrast = 1.0f;
    float depthOffset = 0.0f;
    float depthScale = 1.0f;

    // Visualization
    ViewMode viewMode = ViewMode::Original;
    ColorMapType colorMap = ColorMapType::Turbo;

    // Mask
    bool enableMask = false;
    float maskMin = 0.3f;
    float maskMax = 0.8f;
    float maskSoftness = 0.1f;
    float maskFeather = 0.05f;
    bool maskInvert = false;

    // Depth of Field
    bool enableDoF = false;
    Point2D focusPoint = { 0.5f, 0.5f };
    float focusDepth = 0.5f;
    bool autoSampleFocus = true;
    float focusRange = 0.15f;
    float blurStrength = 15.0f;
    float bokehAmount = 0.5f;
    BokehShape bokehShape = BokehShape::Disk;
    float highlightBoost = 0.2f;
    float fgBlur = 1.0f;
    float bgBlur = 1.0f;

    // 3D Parallax
    bool enableParallax = false;
    float parallaxX = 0.0f;
    float parallaxY = 0.0f;
    float depthStrength = 1.0f;
    float cameraDistance = 2.0f;
    EdgeFillMode edgeFill = EdgeFillMode::Mirror;

    // Depth Zoom
    bool enableDepthZoom = false;
    float nearScale = 1.1f;
    float farScale = 0.95f;
    Point2D zoomCenter = { 0.5f, 0.5f };

    // Depth Fog
    bool enableFog = false;
    float fogAmount = 0.5f;
    float fogStart = 0.2f;
    float fogEnd = 0.9f;
    ColorRGBA fogColor = { 0.8f, 0.85f, 0.9f, 1.0f };
    float fogFalloff = 1.5f;

    // Relighting
    bool enableRelighting = false;
    float lightDirX = 0.5f;
    float lightDirY = -0.5f;
    float lightHeight = 0.8f;
    float lightStrength = 0.8f;
    float lightSoftness = 0.5f;
    float ambientLight = 0.2f;

    // Edge Refinement
    bool enableEdgeRefine = true;
    int edgeRadius = 4;
    float edgeEps = 0.001f;

    // Debug
    bool showPerfOverlay = false;
};

} // namespace AIDepthPro
