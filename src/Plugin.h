#pragma once

#include "openfx/ofxCore.h"
#include "openfx/ofxImageEffect.h"
#include "openfx/ofxParam.h"
#include "openfx/ofxProperty.h"
#include "openfx/ofxMultiThread.h"
#include "openfx/ofxMessage.h"
#include "openfx/ofxMemory.h"

#include "Core/Types.h"
#include "DepthEngine/DepthEngine.h"
#include "Temporal/TemporalStabilizer.h"
#include "Cache/LRUCache.h"
#include "UI/ParameterDefs.h"

#include <memory>
#include <mutex>
#include <limits>
#include <string>

namespace AIDepthPro {

struct HostSuites {
    OfxPropertySuiteV1* propSuite = nullptr;
    OfxParameterSuiteV1* paramSuite = nullptr;
    OfxImageEffectSuiteV1* effectSuite = nullptr;
    OfxMultiThreadSuiteV1* threadSuite = nullptr;
    OfxMessageSuiteV1* messageSuite = nullptr;
    OfxMemorySuiteV1* memorySuite = nullptr;
};

class PluginInstance {
public:
    explicit PluginInstance(OfxImageEffectHandle handle, const HostSuites& suites);
    ~PluginInstance();

    OfxStatus OnInstanceChanged(OfxPropertySetHandle inArgs);
    OfxStatus Render(OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs);
    OfxStatus Purge();
    OfxStatus Sync();

private:
    void fetchParameters(OfxTime time, EffectParams& params);
    uint64_t computeSettingsHash(const EffectParams& params) const;

    OfxImageEffectHandle m_effectHandle;
    HostSuites m_suites;

    // OpenFX Parameter Handles
    OfxParamHandle m_pEngine = nullptr;
    OfxParamHandle m_pQuality = nullptr;
    OfxParamHandle m_pAutoMode = nullptr;
    OfxParamHandle m_pModelPath = nullptr;

    OfxParamHandle m_pTemporalStability = nullptr;
    OfxParamHandle m_pMotionCompensation = nullptr;
    OfxParamHandle m_pFlickerReduction = nullptr;

    OfxParamHandle m_pInvertDepth = nullptr;
    OfxParamHandle m_pNearRange = nullptr;
    OfxParamHandle m_pFarRange = nullptr;
    OfxParamHandle m_pDepthGamma = nullptr;
    OfxParamHandle m_pDepthContrast = nullptr;
    OfxParamHandle m_pDepthOffset = nullptr;
    OfxParamHandle m_pDepthScale = nullptr;

    OfxParamHandle m_pViewMode = nullptr;
    OfxParamHandle m_pColorMap = nullptr;

    OfxParamHandle m_pEnableMask = nullptr;
    OfxParamHandle m_pMaskMin = nullptr;
    OfxParamHandle m_pMaskMax = nullptr;
    OfxParamHandle m_pMaskSoftness = nullptr;
    OfxParamHandle m_pMaskFeather = nullptr;
    OfxParamHandle m_pMaskInvert = nullptr;

    OfxParamHandle m_pEnableDoF = nullptr;
    OfxParamHandle m_pFocusPoint = nullptr;
    OfxParamHandle m_pFocusDepth = nullptr;
    OfxParamHandle m_pAutoSampleFocus = nullptr;
    OfxParamHandle m_pFocusRange = nullptr;
    OfxParamHandle m_pBlurStrength = nullptr;
    OfxParamHandle m_pBokehAmount = nullptr;
    OfxParamHandle m_pBokehShape = nullptr;
    OfxParamHandle m_pHighlightBoost = nullptr;
    OfxParamHandle m_pFgBlur = nullptr;
    OfxParamHandle m_pBgBlur = nullptr;

    OfxParamHandle m_pEnableParallax = nullptr;
    OfxParamHandle m_pParallaxX = nullptr;
    OfxParamHandle m_pParallaxY = nullptr;
    OfxParamHandle m_pDepthStrength = nullptr;
    OfxParamHandle m_pCameraDistance = nullptr;
    OfxParamHandle m_pEdgeFill = nullptr;

    OfxParamHandle m_pEnableDepthZoom = nullptr;
    OfxParamHandle m_pNearScale = nullptr;
    OfxParamHandle m_pFarScale = nullptr;
    OfxParamHandle m_pZoomCenter = nullptr;

    OfxParamHandle m_pEnableFog = nullptr;
    OfxParamHandle m_pFogAmount = nullptr;
    OfxParamHandle m_pFogStart = nullptr;
    OfxParamHandle m_pFogEnd = nullptr;
    OfxParamHandle m_pFogColor = nullptr;
    OfxParamHandle m_pFogFalloff = nullptr;

    OfxParamHandle m_pEnableRelighting = nullptr;
    OfxParamHandle m_pLightDirX = nullptr;
    OfxParamHandle m_pLightDirY = nullptr;
    OfxParamHandle m_pLightHeight = nullptr;
    OfxParamHandle m_pLightStrength = nullptr;
    OfxParamHandle m_pLightSoftness = nullptr;
    OfxParamHandle m_pAmbientLight = nullptr;

    OfxParamHandle m_pEnableEdgeRefine = nullptr;
    OfxParamHandle m_pEdgeRadius = nullptr;
    OfxParamHandle m_pEdgeEps = nullptr;

    OfxParamHandle m_pShowPerfOverlay = nullptr;
    OfxParamHandle m_pClearCache = nullptr;
    OfxParamHandle m_pPerfStatsLabel = nullptr;

    // Internal Subsystems
    std::unique_ptr<DepthEngine> m_depthEngine;
    TemporalStabilizer m_temporalStabilizer;
    LRUCache m_cache;

    EngineConfig m_lastEngineConfig;
    std::mutex m_renderMutex;
    std::vector<uint8_t> m_sourceBuffer, m_outputBuffer, m_scratchBuffer;
    double m_lastTime = std::numeric_limits<double>::quiet_NaN();
    uint64_t m_lastProcessingHash = 0;
};

} // namespace AIDepthPro
