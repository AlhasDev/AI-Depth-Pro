#include "Plugin.h"
#include "Core/MathUtils.h"
#include "Processing/EdgeRefinement.h"
#include "Processing/DepthNormalizer.h"
#include "Processing/ColorMap.h"
#include "Effects/DepthMask.h"
#include "Effects/DepthOfField.h"
#include "Effects/Parallax.h"
#include "Effects/DepthZoom.h"
#include "Effects/DepthFog.h"
#include "Effects/Relighting.h"
#include "UI/OverlayRenderer.h"
#include "Utils/Logger.h"
#include "Utils/FileUtils.h"
#include "Cache/PersistentDepthCache.h"
#include <chrono>
#include <sstream>
#include <cstring>
#include <exception>
#include <cmath>
#include <limits>
#include <filesystem>

namespace AIDepthPro {

PluginInstance::PluginInstance(OfxImageEffectHandle handle, const HostSuites& suites)
    : m_effectHandle(handle), m_suites(suites), m_cache(128, 2048) {
    
    OfxParamSetHandle paramSet = nullptr;
    if (m_suites.effectSuite) {
        m_suites.effectSuite->getParamSet(m_effectHandle, &paramSet);
    }

    if (paramSet && m_suites.paramSuite) {
        auto getHandle = [&](const char* name, OfxParamHandle& ph) {
            m_suites.paramSuite->paramGetHandle(paramSet, name, &ph, nullptr);
        };

        getHandle(Params::kParamEngine, m_pEngine);
        getHandle(Params::kParamQuality, m_pQuality);
        getHandle(Params::kParamAutoMode, m_pAutoMode);
        getHandle(Params::kParamModelPath, m_pModelPath);

        getHandle(Params::kParamTemporalStability, m_pTemporalStability);
        getHandle(Params::kParamMotionCompensation, m_pMotionCompensation);
        getHandle(Params::kParamFlickerReduction, m_pFlickerReduction);

        getHandle(Params::kParamInvertDepth, m_pInvertDepth);
        getHandle(Params::kParamNearRange, m_pNearRange);
        getHandle(Params::kParamFarRange, m_pFarRange);
        getHandle(Params::kParamDepthGamma, m_pDepthGamma);
        getHandle(Params::kParamDepthContrast, m_pDepthContrast);
        getHandle(Params::kParamDepthOffset, m_pDepthOffset);
        getHandle(Params::kParamDepthScale, m_pDepthScale);

        getHandle(Params::kParamViewMode, m_pViewMode);
        getHandle(Params::kParamColorMap, m_pColorMap);

        getHandle(Params::kParamEnableMask, m_pEnableMask);
        getHandle(Params::kParamMaskMin, m_pMaskMin);
        getHandle(Params::kParamMaskMax, m_pMaskMax);
        getHandle(Params::kParamMaskSoftness, m_pMaskSoftness);
        getHandle(Params::kParamMaskFeather, m_pMaskFeather);
        getHandle(Params::kParamMaskInvert, m_pMaskInvert);

        getHandle(Params::kParamEnableDoF, m_pEnableDoF);
        getHandle(Params::kParamFocusPoint, m_pFocusPoint);
        getHandle(Params::kParamFocusDepth, m_pFocusDepth);
        getHandle(Params::kParamAutoSampleFocus, m_pAutoSampleFocus);
        getHandle(Params::kParamFocusRange, m_pFocusRange);
        getHandle(Params::kParamBlurStrength, m_pBlurStrength);
        getHandle(Params::kParamBokehAmount, m_pBokehAmount);
        getHandle(Params::kParamBokehShape, m_pBokehShape);
        getHandle(Params::kParamHighlightBoost, m_pHighlightBoost);
        getHandle(Params::kParamFgBlur, m_pFgBlur);
        getHandle(Params::kParamBgBlur, m_pBgBlur);

        getHandle(Params::kParamEnableParallax, m_pEnableParallax);
        getHandle(Params::kParamParallaxX, m_pParallaxX);
        getHandle(Params::kParamParallaxY, m_pParallaxY);
        getHandle(Params::kParamDepthStrength, m_pDepthStrength);
        getHandle(Params::kParamCameraDistance, m_pCameraDistance);
        getHandle(Params::kParamEdgeFill, m_pEdgeFill);

        getHandle(Params::kParamEnableDepthZoom, m_pEnableDepthZoom);
        getHandle(Params::kParamNearScale, m_pNearScale);
        getHandle(Params::kParamFarScale, m_pFarScale);
        getHandle(Params::kParamZoomCenter, m_pZoomCenter);

        getHandle(Params::kParamEnableFog, m_pEnableFog);
        getHandle(Params::kParamFogAmount, m_pFogAmount);
        getHandle(Params::kParamFogStart, m_pFogStart);
        getHandle(Params::kParamFogEnd, m_pFogEnd);
        getHandle(Params::kParamFogColor, m_pFogColor);
        getHandle(Params::kParamFogFalloff, m_pFogFalloff);

        getHandle(Params::kParamEnableRelighting, m_pEnableRelighting);
        getHandle(Params::kParamLightDirX, m_pLightDirX);
        getHandle(Params::kParamLightDirY, m_pLightDirY);
        getHandle(Params::kParamLightHeight, m_pLightHeight);
        getHandle(Params::kParamLightStrength, m_pLightStrength);
        getHandle(Params::kParamLightSoftness, m_pLightSoftness);
        getHandle(Params::kParamAmbientLight, m_pAmbientLight);

        getHandle(Params::kParamEnableEdgeRefine, m_pEnableEdgeRefine);
        getHandle(Params::kParamEdgeRadius, m_pEdgeRadius);
        getHandle(Params::kParamEdgeEps, m_pEdgeEps);

        getHandle(Params::kParamShowPerfOverlay, m_pShowPerfOverlay);
        getHandle(Params::kParamClearCache, m_pClearCache);
        getHandle(Params::kParamPerfStatsLabel, m_pPerfStatsLabel);
    }

    LOG_INFO("AI Depth Pro plugin instance created successfully.");
}

PluginInstance::~PluginInstance() {
    try {
        if (m_depthEngine) {
            m_depthEngine->Shutdown();
        }
        m_cache.clear();
    } catch (...) {}
    LOG_INFO("AI Depth Pro plugin instance destroyed.");
}

OfxStatus PluginInstance::Purge() {
    std::lock_guard<std::mutex> lock(m_renderMutex);
    m_lastTime = std::numeric_limits<double>::quiet_NaN();
    m_cache.clear();
    m_temporalStabilizer.Reset();
    return kOfxStatOK;
}

OfxStatus PluginInstance::Sync() {
    return kOfxStatOK;
}

OfxStatus PluginInstance::OnInstanceChanged(OfxPropertySetHandle inArgs) {
        std::lock_guard<std::mutex> lock(m_renderMutex);
    char* name = nullptr;
    if (inArgs) m_suites.propSuite->propGetString(inArgs, kOfxPropName, 0, &name);
    if (name && (std::strcmp(name, Params::kParamClearCache) == 0 ||
                 std::strcmp(name, "Source") == 0 ||
                 std::strcmp(name, Params::kParamModelPath) == 0)) {
        m_cache.clear();
        m_temporalStabilizer.Reset();
        m_lastTime = std::numeric_limits<double>::quiet_NaN();
        if (std::strcmp(name, Params::kParamModelPath) == 0) m_depthEngine.reset();
        if (std::strcmp(name, Params::kParamClearCache) == 0) PersistentDepthCache::clear();
    }
    return kOfxStatOK;
}

void PluginInstance::fetchParameters(OfxTime time, EffectParams& params) {
    int intVal = 0;
    double dblVal = 0.0;
    double dblVal2[2] = { 0.0, 0.0 };
    double dblVal4[4] = { 0.0, 0.0, 0.0, 0.0 };
    char* strVal = nullptr;

    if (!m_suites.paramSuite) return;

    if (m_pEngine) { m_suites.paramSuite->paramGetValueAtTime(m_pEngine, time, &intVal); params.engine = static_cast<EngineType>(intVal); }
    if (m_pQuality) { m_suites.paramSuite->paramGetValueAtTime(m_pQuality, time, &intVal); params.quality = static_cast<QualityMode>(intVal); }
    if (m_pAutoMode) { m_suites.paramSuite->paramGetValueAtTime(m_pAutoMode, time, &intVal); params.autoMode = (intVal != 0); }
    if (m_pModelPath) {
        m_suites.paramSuite->paramGetValueAtTime(m_pModelPath, time, &strVal);
        if (strVal) params.customModelPath = strVal;
    }

    if (m_pTemporalStability) { m_suites.paramSuite->paramGetValueAtTime(m_pTemporalStability, time, &dblVal); params.temporalStability = static_cast<float>(dblVal); }
    if (m_pMotionCompensation) { m_suites.paramSuite->paramGetValueAtTime(m_pMotionCompensation, time, &intVal); params.motionCompensation = (intVal != 0); }
    if (m_pFlickerReduction) { m_suites.paramSuite->paramGetValueAtTime(m_pFlickerReduction, time, &dblVal); params.flickerReduction = static_cast<float>(dblVal); }

    if (m_pInvertDepth) { m_suites.paramSuite->paramGetValueAtTime(m_pInvertDepth, time, &intVal); params.invertDepth = (intVal != 0); }
    if (m_pNearRange) { m_suites.paramSuite->paramGetValueAtTime(m_pNearRange, time, &dblVal); params.nearRange = static_cast<float>(dblVal); }
    if (m_pFarRange) { m_suites.paramSuite->paramGetValueAtTime(m_pFarRange, time, &dblVal); params.farRange = static_cast<float>(dblVal); }
    if (m_pDepthGamma) { m_suites.paramSuite->paramGetValueAtTime(m_pDepthGamma, time, &dblVal); params.depthGamma = static_cast<float>(dblVal); }
    if (m_pDepthContrast) { m_suites.paramSuite->paramGetValueAtTime(m_pDepthContrast, time, &dblVal); params.depthContrast = static_cast<float>(dblVal); }
    if (m_pDepthOffset) { m_suites.paramSuite->paramGetValueAtTime(m_pDepthOffset, time, &dblVal); params.depthOffset = static_cast<float>(dblVal); }
    if (m_pDepthScale) { m_suites.paramSuite->paramGetValueAtTime(m_pDepthScale, time, &dblVal); params.depthScale = static_cast<float>(dblVal); }

    if (m_pViewMode) { m_suites.paramSuite->paramGetValueAtTime(m_pViewMode, time, &intVal); params.viewMode = static_cast<ViewMode>(intVal); }
    if (m_pColorMap) { m_suites.paramSuite->paramGetValueAtTime(m_pColorMap, time, &intVal); params.colorMap = static_cast<ColorMapType>(intVal); }

    if (m_pEnableMask) { m_suites.paramSuite->paramGetValueAtTime(m_pEnableMask, time, &intVal); params.enableMask = (intVal != 0); }
    if (m_pMaskMin) { m_suites.paramSuite->paramGetValueAtTime(m_pMaskMin, time, &dblVal); params.maskMin = static_cast<float>(dblVal); }
    if (m_pMaskMax) { m_suites.paramSuite->paramGetValueAtTime(m_pMaskMax, time, &dblVal); params.maskMax = static_cast<float>(dblVal); }
    if (m_pMaskSoftness) { m_suites.paramSuite->paramGetValueAtTime(m_pMaskSoftness, time, &dblVal); params.maskSoftness = static_cast<float>(dblVal); }
    if (m_pMaskFeather) { m_suites.paramSuite->paramGetValueAtTime(m_pMaskFeather, time, &dblVal); params.maskFeather = static_cast<float>(dblVal); }
    if (m_pMaskInvert) { m_suites.paramSuite->paramGetValueAtTime(m_pMaskInvert, time, &intVal); params.maskInvert = (intVal != 0); }

    if (m_pEnableDoF) { m_suites.paramSuite->paramGetValueAtTime(m_pEnableDoF, time, &intVal); params.enableDoF = (intVal != 0); }
    if (m_pFocusPoint) { m_suites.paramSuite->paramGetValueAtTime(m_pFocusPoint, time, &dblVal2[0], &dblVal2[1]); params.focusPoint = { static_cast<float>(dblVal2[0]), static_cast<float>(dblVal2[1]) }; }
    if (m_pFocusDepth) { m_suites.paramSuite->paramGetValueAtTime(m_pFocusDepth, time, &dblVal); params.focusDepth = static_cast<float>(dblVal); }
    if (m_pAutoSampleFocus) { m_suites.paramSuite->paramGetValueAtTime(m_pAutoSampleFocus, time, &intVal); params.autoSampleFocus = (intVal != 0); }
    if (m_pFocusRange) { m_suites.paramSuite->paramGetValueAtTime(m_pFocusRange, time, &dblVal); params.focusRange = static_cast<float>(dblVal); }
    if (m_pBlurStrength) { m_suites.paramSuite->paramGetValueAtTime(m_pBlurStrength, time, &dblVal); params.blurStrength = static_cast<float>(dblVal); }
    if (m_pBokehAmount) { m_suites.paramSuite->paramGetValueAtTime(m_pBokehAmount, time, &dblVal); params.bokehAmount = static_cast<float>(dblVal); }
    if (m_pBokehShape) { m_suites.paramSuite->paramGetValueAtTime(m_pBokehShape, time, &intVal); params.bokehShape = static_cast<BokehShape>(intVal); }
    if (m_pHighlightBoost) { m_suites.paramSuite->paramGetValueAtTime(m_pHighlightBoost, time, &dblVal); params.highlightBoost = static_cast<float>(dblVal); }
    if (m_pFgBlur) { m_suites.paramSuite->paramGetValueAtTime(m_pFgBlur, time, &dblVal); params.fgBlur = static_cast<float>(dblVal); }
    if (m_pBgBlur) { m_suites.paramSuite->paramGetValueAtTime(m_pBgBlur, time, &dblVal); params.bgBlur = static_cast<float>(dblVal); }

    if (m_pEnableParallax) { m_suites.paramSuite->paramGetValueAtTime(m_pEnableParallax, time, &intVal); params.enableParallax = (intVal != 0); }
    if (m_pParallaxX) { m_suites.paramSuite->paramGetValueAtTime(m_pParallaxX, time, &dblVal); params.parallaxX = static_cast<float>(dblVal); }
    if (m_pParallaxY) { m_suites.paramSuite->paramGetValueAtTime(m_pParallaxY, time, &dblVal); params.parallaxY = static_cast<float>(dblVal); }
    if (m_pDepthStrength) { m_suites.paramSuite->paramGetValueAtTime(m_pDepthStrength, time, &dblVal); params.depthStrength = static_cast<float>(dblVal); }
    if (m_pCameraDistance) { m_suites.paramSuite->paramGetValueAtTime(m_pCameraDistance, time, &dblVal); params.cameraDistance = static_cast<float>(dblVal); }
    if (m_pEdgeFill) { m_suites.paramSuite->paramGetValueAtTime(m_pEdgeFill, time, &intVal); params.edgeFill = static_cast<EdgeFillMode>(intVal); }

    if (m_pEnableDepthZoom) { m_suites.paramSuite->paramGetValueAtTime(m_pEnableDepthZoom, time, &intVal); params.enableDepthZoom = (intVal != 0); }
    if (m_pNearScale) { m_suites.paramSuite->paramGetValueAtTime(m_pNearScale, time, &dblVal); params.nearScale = static_cast<float>(dblVal); }
    if (m_pFarScale) { m_suites.paramSuite->paramGetValueAtTime(m_pFarScale, time, &dblVal); params.farScale = static_cast<float>(dblVal); }
    if (m_pZoomCenter) { m_suites.paramSuite->paramGetValueAtTime(m_pZoomCenter, time, &dblVal2[0], &dblVal2[1]); params.zoomCenter = { static_cast<float>(dblVal2[0]), static_cast<float>(dblVal2[1]) }; }

    if (m_pEnableFog) { m_suites.paramSuite->paramGetValueAtTime(m_pEnableFog, time, &intVal); params.enableFog = (intVal != 0); }
    if (m_pFogAmount) { m_suites.paramSuite->paramGetValueAtTime(m_pFogAmount, time, &dblVal); params.fogAmount = static_cast<float>(dblVal); }
    if (m_pFogStart) { m_suites.paramSuite->paramGetValueAtTime(m_pFogStart, time, &dblVal); params.fogStart = static_cast<float>(dblVal); }
    if (m_pFogEnd) { m_suites.paramSuite->paramGetValueAtTime(m_pFogEnd, time, &dblVal); params.fogEnd = static_cast<float>(dblVal); }
    if (m_pFogColor) {
        m_suites.paramSuite->paramGetValueAtTime(m_pFogColor, time, &dblVal4[0], &dblVal4[1], &dblVal4[2], &dblVal4[3]);
        params.fogColor = { static_cast<float>(dblVal4[0]), static_cast<float>(dblVal4[1]), static_cast<float>(dblVal4[2]), static_cast<float>(dblVal4[3]) };
    }
    if (m_pFogFalloff) { m_suites.paramSuite->paramGetValueAtTime(m_pFogFalloff, time, &dblVal); params.fogFalloff = static_cast<float>(dblVal); }

    if (m_pEnableRelighting) { m_suites.paramSuite->paramGetValueAtTime(m_pEnableRelighting, time, &intVal); params.enableRelighting = (intVal != 0); }
    if (m_pLightDirX) { m_suites.paramSuite->paramGetValueAtTime(m_pLightDirX, time, &dblVal); params.lightDirX = static_cast<float>(dblVal); }
    if (m_pLightDirY) { m_suites.paramSuite->paramGetValueAtTime(m_pLightDirY, time, &dblVal); params.lightDirY = static_cast<float>(dblVal); }
    if (m_pLightHeight) { m_suites.paramSuite->paramGetValueAtTime(m_pLightHeight, time, &dblVal); params.lightHeight = static_cast<float>(dblVal); }
    if (m_pLightStrength) { m_suites.paramSuite->paramGetValueAtTime(m_pLightStrength, time, &dblVal); params.lightStrength = static_cast<float>(dblVal); }
    if (m_pLightSoftness) { m_suites.paramSuite->paramGetValueAtTime(m_pLightSoftness, time, &dblVal); params.lightSoftness = static_cast<float>(dblVal); }
    if (m_pAmbientLight) { m_suites.paramSuite->paramGetValueAtTime(m_pAmbientLight, time, &dblVal); params.ambientLight = static_cast<float>(dblVal); }

    if (m_pEnableEdgeRefine) { m_suites.paramSuite->paramGetValueAtTime(m_pEnableEdgeRefine, time, &intVal); params.enableEdgeRefine = (intVal != 0); }
    if (m_pEdgeRadius) { m_suites.paramSuite->paramGetValueAtTime(m_pEdgeRadius, time, &intVal); params.edgeRadius = intVal; }
    if (m_pEdgeEps) { m_suites.paramSuite->paramGetValueAtTime(m_pEdgeEps, time, &dblVal); params.edgeEps = static_cast<float>(dblVal); }

    if (m_pShowPerfOverlay) { m_suites.paramSuite->paramGetValueAtTime(m_pShowPerfOverlay, time, &intVal); params.showPerfOverlay = (intVal != 0); }
}

uint64_t PluginInstance::computeSettingsHash(const EffectParams& p) const {
    uint64_t hash = 0x811c9dc5;
    auto addFloat = [&](float f) {
        uint32_t val;
        std::memcpy(&val, &f, sizeof(float));
        hash = (hash ^ val) * 0x01000193;
    };
    auto addInt = [&](int i) {
        hash = (hash ^ static_cast<uint32_t>(i)) * 0x01000193;
    };

    addInt(static_cast<int>(p.engine));
    addInt(static_cast<int>(p.quality));
    addFloat(p.temporalStability);
    addInt(p.enableEdgeRefine ? 1 : 0);
    addInt(p.edgeRadius);
    addFloat(p.edgeEps);
    addInt(p.motionCompensation ? 1 : 0);
    addFloat(p.flickerReduction);
    return hash;
}

namespace {
struct ImageLease {
    OfxImageEffectSuiteV1* suite;
    OfxPropertySetHandle image = nullptr;
    ~ImageLease() { if (image) suite->clipReleaseImage(image); }
};
std::string fingerprint(const ImageFrame& frame) {
    uint64_t hash = 14695981039346656037ull;
    size_t bytes = size_t(frame.width) * (frame.components == PixelComponent::RGBA ? 4 : 3) *
        (frame.bitDepth == BitDepth::Float ? sizeof(float) : 1);
    for (int y = 0; y < frame.height; ++y) {
        const auto* row = static_cast<const uint8_t*>(frame.data) + y * frame.rowBytes;
        for (size_t i = 0; i < bytes; ++i) { hash ^= row[i]; hash *= 1099511628211ull; }
    }
    return std::to_string(hash);
}
void copyRows(const ImageFrame& src, ImageFrame& dst) {
    const size_t bytes = size_t(dst.width) * (dst.components == PixelComponent::RGBA ? 4 : 3) *
        (dst.bitDepth == BitDepth::Float ? sizeof(float) : 1);
    for (int y = 0; y < dst.height; ++y)
        std::memcpy(static_cast<char*>(dst.data) + y * dst.rowBytes,
                    static_cast<const char*>(src.data) + y * src.rowBytes, bytes);
}
}
OfxStatus PluginInstance::Render(OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs) {
    (void)outArgs;
    std::lock_guard<std::mutex> lock(m_renderMutex);
    if (!m_suites.effectSuite || !m_suites.propSuite) return kOfxStatErrMissingHostFeature;
    try {
        const auto begin = std::chrono::steady_clock::now();
        auto* suite = m_suites.effectSuite;
        auto* prop = m_suites.propSuite;
        double time = 0;
        prop->propGetDouble(inArgs, kOfxPropTime, 0, &time);
        EffectParams params;
        fetchParameters(time, params);
        OfxImageClipHandle source = nullptr, destination = nullptr;
        if (suite->clipGetHandle(m_effectHandle, "Source", &source, nullptr) != kOfxStatOK ||
            suite->clipGetHandle(m_effectHandle, "Output", &destination, nullptr) != kOfxStatOK)
            return kOfxStatErrBadHandle;
        ImageLease src{suite}, dst{suite};
        if (suite->clipGetImage(source, time, nullptr, &src.image) != kOfxStatOK ||
            suite->clipGetImage(destination, time, nullptr, &dst.image) != kOfxStatOK)
            return kOfxStatFailed;
        void* srcData = nullptr; void* dstData = nullptr;
        OfxRectI sb{}, db{};
        int srcStride = 0, dstStride = 0;
        char *depth = nullptr, *components = nullptr, *outDepth = nullptr, *outComponents = nullptr;
        if (prop->propGetPointer(src.image, kOfxImagePropData, 0, &srcData) != kOfxStatOK ||
            prop->propGetPointer(dst.image, kOfxImagePropData, 0, &dstData) != kOfxStatOK ||
            prop->propGetIntN(src.image, kOfxImagePropBounds, 4, &sb.x1) != kOfxStatOK ||
            prop->propGetIntN(dst.image, kOfxImagePropBounds, 4, &db.x1) != kOfxStatOK ||
            prop->propGetInt(src.image, kOfxImagePropRowBytes, 0, &srcStride) != kOfxStatOK ||
            prop->propGetInt(dst.image, kOfxImagePropRowBytes, 0, &dstStride) != kOfxStatOK)
            return kOfxStatErrFormat;
        prop->propGetString(src.image, kOfxImageEffectPropPixelDepth, 0, &depth);
        prop->propGetString(dst.image, kOfxImageEffectPropPixelDepth, 0, &outDepth);
        prop->propGetString(src.image, kOfxImageEffectPropComponents, 0, &components);
        prop->propGetString(dst.image, kOfxImageEffectPropComponents, 0, &outComponents);
        if (!depth || !components || !outDepth || !outComponents || std::strcmp(depth,outDepth) ||
            std::strcmp(components,outComponents) || !srcData || !dstData) return kOfxStatErrFormat;
        const bool fp = std::strcmp(depth, kOfxBitDepthFloat) == 0;
        if (!fp && std::strcmp(depth,kOfxBitDepthByte)) return kOfxStatErrUnsupported;
        const bool rgba = std::strcmp(components,kOfxImageComponentRGBA) == 0;
        if (!rgba && std::strcmp(components,kOfxImageComponentRGB)) return kOfxStatErrUnsupported;
        if (sb.x1 != db.x1 || sb.y1 != db.y1 || sb.x2 != db.x2 || sb.y2 != db.y2) return kOfxStatErrUnsupported;
        const int w = sb.x2 - sb.x1, h = sb.y2 - sb.y1;
        if (w <= 0 || h <= 0) return kOfxStatErrFormat;
        const size_t pixelBytes = (rgba ? 4 : 3) * (fp ? sizeof(float) : 1);
        const size_t rowBytes = size_t(w) * pixelBytes;
        if (std::abs(int64_t(srcStride)) < int64_t(rowBytes) || std::abs(int64_t(dstStride)) < int64_t(rowBytes))
            return kOfxStatErrFormat;
        ImageFrame input{srcData,w,h,size_t(std::abs(int64_t(srcStride))),fp ? BitDepth::Float : BitDepth::Byte,
                         rgba ? PixelComponent::RGBA : PixelComponent::RGB};
        if (srcStride < 0) {
            m_sourceBuffer.resize(rowBytes * h);
            for (int y=0;y<h;++y) std::memcpy(m_sourceBuffer.data()+y*rowBytes,
                static_cast<const char*>(srcData)+ptrdiff_t(y)*srcStride,rowBytes);
            input.data=m_sourceBuffer.data(); input.rowBytes=rowBytes;
        }
        OfxRectI window = db;
        prop->propGetIntN(inArgs,kOfxImageEffectPropRenderWindow,4,&window.x1);
        window.x1=std::max(window.x1,db.x1); window.y1=std::max(window.y1,db.y1);
        window.x2=std::min(window.x2,db.x2); window.y2=std::min(window.y2,db.y2);
        auto commit = [&](const ImageFrame& frame) {
            if (window.x2<=window.x1 || window.y2<=window.y1) return;
            const size_t offset = size_t(window.x1-db.x1)*pixelBytes;
            const size_t bytes = size_t(window.x2-window.x1)*pixelBytes;
            for(int y=window.y1-db.y1;y<window.y2-db.y1;++y)
                std::memcpy(static_cast<char*>(dstData)+ptrdiff_t(y)*dstStride+offset,
                    static_cast<const char*>(frame.data)+y*frame.rowBytes+offset,bytes);
        };
        // Identity is a row copy: no inference, filtering or full-frame scratch allocation.
        if (params.viewMode == ViewMode::Original && !params.enableDoF && !params.enableParallax &&
            !params.enableDepthZoom && !params.enableFog && !params.enableRelighting && !params.enableMask &&
            !params.showPerfOverlay) {
            m_temporalStabilizer.Reset();
            m_lastTime = std::numeric_limits<double>::quiet_NaN();
            commit(input); return kOfxStatOK;
        }
        EngineConfig config;
        config.engineType=params.engine; config.qualityMode=params.quality; config.modelPath=params.customModelPath;
        if (!m_depthEngine || m_lastEngineConfig.engineType!=config.engineType ||
            m_lastEngineConfig.qualityMode!=config.qualityMode || m_lastEngineConfig.modelPath!=config.modelPath) {
            m_depthEngine=DepthEngineFactory::Create(params.engine);
            m_cache.clear(); m_temporalStabilizer.Reset();
            m_lastTime=std::numeric_limits<double>::quiet_NaN();
            if (!m_depthEngine || !m_depthEngine->Initialize(config)) {
                m_depthEngine.reset();
                if (m_suites.messageSuite) m_suites.messageSuite->message(m_effectHandle,kOfxMessageError,
                    "depth_init","%s","Depth model could not load. Check the model, ONNX Runtime and selected backend.");
                return kOfxStatFailed;
            }
            m_lastEngineConfig=config;
        }
        CacheKey key;
        key.time=time; key.width=w; key.height=h; key.quality=params.quality;
        key.settingsHash=static_cast<uint64_t>(params.engine);
        const std::string requestedModel = params.customModelPath.empty() ? "depth_anything_v2_vits.onnx" : params.customModelPath;
        const std::string resolvedModel = FileUtils::findModelFile(requestedModel);
        key.modelPath = resolvedModel.empty() ? requestedModel : resolvedModel;
        if (!resolvedModel.empty()) {
            std::error_code ec;
            const auto size = std::filesystem::file_size(resolvedModel, ec);
            if (!ec) key.modelPath += ":" + std::to_string(size);
            ec.clear();
            const auto modified = std::filesystem::last_write_time(resolvedModel, ec);
            if (!ec) key.modelPath += ":" + std::to_string(modified.time_since_epoch().count());
        }
        key.sourceId=fingerprint(input);
        DepthFrame raw;
        if (!m_cache.get(key,raw)) {
            if (!PersistentDepthCache::load(key,raw)) {
                if (!m_depthEngine->GenerateDepth(input,raw) || !raw.isValid()) return kOfxStatFailed;
                PersistentDepthCache::store(key,raw);
            }
            m_cache.put(key,raw); // Cache inference only; edits to postprocessing reuse the model result.
        }
        const uint64_t processingHash=computeSettingsHash(params);
        if (!std::isfinite(m_lastTime) || std::abs(time-m_lastTime-1.0)>1e-6 || processingHash!=m_lastProcessingHash)
            m_temporalStabilizer.Reset();
        if (params.enableEdgeRefine) {
            DepthFrame refined;
            EdgeRefinement::ApplyGuidedFilter(input,raw,refined,params.edgeRadius,params.edgeEps);
            if (refined.isValid()) raw=std::move(refined);
        }
        if (params.temporalStability>0.001f) {
            DepthFrame stable;
            m_temporalStabilizer.Stabilize(input,raw,stable,params.temporalStability,params.motionCompensation,params.flickerReduction);
            raw=std::move(stable);
        }
        m_lastTime=time; m_lastProcessingHash=processingHash;
        DepthFrame adjusted;
        DepthNormalizer::Process(raw,adjusted,params);
        m_outputBuffer.resize(rowBytes*h);
        ImageFrame output{m_outputBuffer.data(),w,h,rowBytes,input.bitDepth,input.components};
        if (params.viewMode!=ViewMode::Original) {
            ColorMap::RenderVisualization(input,adjusted,output,params);
        } else {
            const int effects=int(params.enableDoF)+int(params.enableParallax)+int(params.enableDepthZoom)+
                int(params.enableFog)+int(params.enableRelighting)+int(params.enableMask);
            if (effects>1) m_scratchBuffer.resize(rowBytes*h);
            ImageFrame scratch{m_scratchBuffer.data(),w,h,rowBytes,input.bitDepth,input.components};
            ImageFrame inStage=input,outStage=output;
            auto advance=[&] { inStage=outStage; outStage=(outStage.data==output.data ? scratch : output); };
            if (params.enableDoF) { DepthOfField::ApplyDoF(inStage,adjusted,outStage,params); advance(); }
            if (params.enableParallax) { Parallax::ApplyParallax(inStage,adjusted,outStage,params); advance(); }
            if (params.enableDepthZoom) { DepthZoom::ApplyDepthZoom(inStage,adjusted,outStage,params); advance(); }
            if (params.enableFog) { DepthFog::ApplyFog(inStage,adjusted,outStage,params); advance(); }
            if (params.enableRelighting) { Relighting::ApplyRelighting(inStage,adjusted,outStage,params); advance(); }
            if (params.enableMask) {
                std::vector<float> mask;
                DepthMask::GenerateMask(adjusted,mask,params.maskMin,params.maskMax,params.maskSoftness,params.maskFeather,params.maskInvert);
                DepthMask::ApplyMask(inStage,mask,outStage); advance();
            }
            if (inStage.data!=output.data) copyRows(inStage,output);
        }
        if (params.showPerfOverlay) {
            if (params.enableDoF && params.autoSampleFocus) OverlayRenderer::DrawFocusTarget(output,params.focusPoint);
            const float ms=std::chrono::duration<float,std::milli>(std::chrono::steady_clock::now()-begin).count();
            OverlayRenderer::DrawPerformanceOverlay(output,m_depthEngine->GetStats(),ms,m_cache.getFrameCount(),m_cache.getHitRate());
        }
        if (suite->abort && suite->abort(m_effectHandle)) return kOfxStatFailed;
        commit(output);
        return kOfxStatOK;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Render failed: ")+e.what());
        return kOfxStatFailed;
    } catch (...) { return kOfxStatFailed; }
}
} // namespace AIDepthPro
