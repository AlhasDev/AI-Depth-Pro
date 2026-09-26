#include "Plugin.h"
#include "openfx/ofxCore.h"
#include "openfx/ofxImageEffect.h"
#include "openfx/ofxParam.h"
#include "openfx/ofxProperty.h"
#include "openfx/ofxMultiThread.h"
#include "openfx/ofxMessage.h"
#include "openfx/ofxMemory.h"
#include "Utils/Logger.h"
#include <cstring>
#include <iostream>

using namespace AIDepthPro;

static HostSuites gSuites;
static OfxHost* gHost = nullptr;

static OfxStatus describePlugin(OfxImageEffectHandle descriptor) {
    OfxPropertySetHandle effectProps = nullptr;
    gSuites.effectSuite->getPropertySet(descriptor, &effectProps);

    // Set plugin properties
    gSuites.propSuite->propSetString(effectProps, kOfxPropLabel, 0, "AI Depth Pro");
    gSuites.propSuite->propSetString(effectProps, kOfxPropShortLabel, 0, "AI Depth");
    gSuites.propSuite->propSetString(effectProps, kOfxPropLongLabel, 0, "AI Depth Pro");
    gSuites.propSuite->propSetString(effectProps, kOfxImageEffectPluginPropGrouping, 0, "AI Depth");
    gSuites.propSuite->propSetString(effectProps, kOfxPropPluginDescription, 0,
        "AI Depth Pro: Real-time AI Depth Map Generator, 2.5D Parallax, Depth of Field & Depth Effects for DaVinci Resolve 21.");

    gSuites.propSuite->propSetInt(effectProps, "OfxImageEffectPropSupportsTiles", 0, 0);
    gSuites.propSuite->propSetInt(effectProps, "OfxImageEffectPropSupportsMultiResolution", 0, 0);
    // Supported Contexts
    const char* contexts[] = {
        kOfxImageEffectContextFilter,
        kOfxImageEffectContextGeneral,
        kOfxImageEffectContextPaint
    };
    gSuites.propSuite->propSetStringN(effectProps, kOfxImageEffectPropSupportedContexts, 3, contexts);

    // Supported Pixel Depths (Float 32-bit native for DaVinci Resolve color science + Byte 8-bit fallback)
    const char* pixelDepths[] = {
        kOfxBitDepthFloat,
        kOfxBitDepthByte
    };
    gSuites.propSuite->propSetStringN(effectProps, kOfxImageEffectPropSupportedPixelDepths, 2, pixelDepths);

    // Thread Safety
    gSuites.propSuite->propSetString(effectProps, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderInstanceSafe);
    gSuites.propSuite->propSetInt(effectProps, kOfxImageEffectPluginPropHostFrameThreading, 0, 0);
    gSuites.propSuite->propSetInt(effectProps, kOfxImageEffectPluginPropSingleInstance, 0, 0);

    return kOfxStatOK;
}

static OfxStatus describePluginInContext(OfxImageEffectHandle descriptor, OfxPropertySetHandle inArgs) {
    (void)inArgs;
    OfxParamSetHandle paramSet = nullptr;
    gSuites.effectSuite->getParamSet(descriptor, &paramSet);

    // 1. Clips definition
    OfxPropertySetHandle srcClipProps = nullptr;
    OfxPropertySetHandle dstClipProps = nullptr;
    gSuites.effectSuite->clipDefine(descriptor, kOfxImageEffectSimpleSourceClipName, &srcClipProps);
    gSuites.effectSuite->clipDefine(descriptor, kOfxImageEffectOutputClipName, &dstClipProps);

    const char* components[] = { kOfxImageComponentRGBA, kOfxImageComponentRGB };
    if (srcClipProps) {
        gSuites.propSuite->propSetStringN(srcClipProps, kOfxImageEffectPropSupportedComponents, 2, components);
        gSuites.propSuite->propSetInt(srcClipProps, kOfxImageClipPropOptional, 0, 0);
    }
    if (dstClipProps) {
        gSuites.propSuite->propSetStringN(dstClipProps, kOfxImageEffectPropSupportedComponents, 2, components);
    }

    // Helper lambda to define parameters
    auto defineParam = [&](const char* type, const char* name, const char* label, const char* group = nullptr) -> OfxPropertySetHandle {
        OfxPropertySetHandle pProps = nullptr;
        gSuites.paramSuite->paramDefine(paramSet, type, name, &pProps);
        if (pProps && label) {
            gSuites.propSuite->propSetString(pProps, kOfxPropLabel, 0, label);
        }
        if (pProps && group) {
            gSuites.propSuite->propSetString(pProps, kOfxParamPropParent, 0, group);
        }
        return pProps;
    };

    // --- ENGINE GROUP ---
    defineParam(kOfxParamTypeGroup, Params::kGroupEngine, "AI Depth Engine");
    {
        OfxPropertySetHandle p = defineParam(kOfxParamTypeChoice, Params::kParamEngine, "Inference Backend", Params::kGroupEngine);
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 0, "Auto (GPU Preferred)");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 1, "CUDA (not available)");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 2, "TensorRT (not available)");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 3, "DirectML (AMD/Intel/NVIDIA)");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 4, "Metal (not available)");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 5, "ONNX CPU");
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 0);

        p = defineParam(kOfxParamTypeChoice, Params::kParamQuality, "Quality Mode", Params::kGroupEngine);
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 0, "Fast (Real-time Playback)");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 1, "Balanced (Recommended)");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 2, "Quality (Production Master)");
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 1);

        p = defineParam(kOfxParamTypeBoolean, Params::kParamAutoMode, "Auto Mode", Params::kGroupEngine);
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 1);

        p = defineParam(kOfxParamTypeString, Params::kParamModelPath, "Custom Model Path", Params::kGroupEngine);
        gSuites.propSuite->propSetString(p, kOfxParamPropStringMode, 0, kOfxParamPropStringFilePath);
        gSuites.propSuite->propSetString(p, kOfxParamPropDefault, 0, "");
    }

    // --- TEMPORAL GROUP ---
    defineParam(kOfxParamTypeGroup, Params::kGroupTemporal, "Temporal Stability");
    {
        OfxPropertySetHandle p = defineParam(kOfxParamTypeDouble, Params::kParamTemporalStability, "Temporal Smoothing", Params::kGroupTemporal);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.5);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 1.0);

        p = defineParam(kOfxParamTypeBoolean, Params::kParamMotionCompensation, "Motion Compensation", Params::kGroupTemporal);
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 1);

        p = defineParam(kOfxParamTypeDouble, Params::kParamFlickerReduction, "Flicker Reduction", Params::kGroupTemporal);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.3);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 1.0);
    }

    // --- DEPTH ADJUST GROUP ---
    defineParam(kOfxParamTypeGroup, Params::kGroupDepthAdjust, "Depth Adjustments");
    {
        OfxPropertySetHandle p = defineParam(kOfxParamTypeBoolean, Params::kParamInvertDepth, "Invert Depth (White=Far)", Params::kGroupDepthAdjust);
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamNearRange, "Near Range", Params::kGroupDepthAdjust);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 1.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamFarRange, "Far Range", Params::kGroupDepthAdjust);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 1.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 1.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamDepthGamma, "Depth Gamma", Params::kGroupDepthAdjust);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 1.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.2);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 3.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamDepthContrast, "Depth Contrast", Params::kGroupDepthAdjust);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 1.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 3.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamDepthOffset, "Depth Offset", Params::kGroupDepthAdjust);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, -1.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 1.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamDepthScale, "Depth Scale", Params::kGroupDepthAdjust);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 1.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 2.0);
    }

    // --- VISUALIZATION GROUP ---
    defineParam(kOfxParamTypeGroup, Params::kGroupVisualization, "Depth Preview & Visualization");
    {
        OfxPropertySetHandle p = defineParam(kOfxParamTypeChoice, Params::kParamViewMode, "View Mode", Params::kGroupVisualization);
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 0, "Original (Pass-Through / Effects)");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 1, "Depth Map (Greyscale)");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 2, "Inverted Depth");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 3, "Near / Far Mask");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 4, "False Color Depth (Turbo/Magma)");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 5, "Edge Map (Depth Discontinuity)");
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 0);

        p = defineParam(kOfxParamTypeChoice, Params::kParamColorMap, "False Color Palette", Params::kGroupVisualization);
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 0, "Turbo (Google High-Dynamic)");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 1, "Magma");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 2, "Inferno");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 3, "Cool-Warm (Blue to Red)");
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 0);
    }

    // --- DEPTH MASK GROUP ---
    defineParam(kOfxParamTypeGroup, Params::kGroupMask, "Depth Mask Isolation");
    {
        OfxPropertySetHandle p = defineParam(kOfxParamTypeBoolean, Params::kParamEnableMask, "Enable Mask", Params::kGroupMask);
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamMaskMin, "Mask Min Depth", Params::kGroupMask);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.3);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 1.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamMaskMax, "Mask Max Depth", Params::kGroupMask);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.8);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 1.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamMaskSoftness, "Mask Softness", Params::kGroupMask);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.1);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 0.5);

        p = defineParam(kOfxParamTypeDouble, Params::kParamMaskFeather, "Mask Feather / Blur", Params::kGroupMask);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.05);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 0.5);

        p = defineParam(kOfxParamTypeBoolean, Params::kParamMaskInvert, "Invert Mask", Params::kGroupMask);
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 0);
    }

    // --- DEPTH OF FIELD GROUP ---
    defineParam(kOfxParamTypeGroup, Params::kGroupDoF, "Depth of Field & Bokeh");
    {
        OfxPropertySetHandle p = defineParam(kOfxParamTypeBoolean, Params::kParamEnableDoF, "Enable Depth of Field", Params::kGroupDoF);
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 0);

        p = defineParam(kOfxParamTypeDouble2D, Params::kParamFocusPoint, "Focus Picker (X, Y)", Params::kGroupDoF);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.5);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 1, 0.5);

        p = defineParam(kOfxParamTypeBoolean, Params::kParamAutoSampleFocus, "Auto Sample Focus from Point", Params::kGroupDoF);
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 1);

        p = defineParam(kOfxParamTypeDouble, Params::kParamFocusDepth, "Manual Focus Depth", Params::kGroupDoF);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.5);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 1.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamFocusRange, "Focal Range (F-Stop)", Params::kGroupDoF);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.15);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.01);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 0.5);

        p = defineParam(kOfxParamTypeDouble, Params::kParamBlurStrength, "Blur Strength / Aperture", Params::kGroupDoF);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 15.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 64.0);

        p = defineParam(kOfxParamTypeChoice, Params::kParamBokehShape, "Bokeh Shape", Params::kGroupDoF);
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 0, "Circular Disc");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 1, "Hexagonal (6 Blades)");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 2, "Smooth Gaussian");
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamHighlightBoost, "Highlight Specular Boost", Params::kGroupDoF);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.2);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 1.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamFgBlur, "Foreground Blur Multiplier", Params::kGroupDoF);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 1.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 2.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamBgBlur, "Background Blur Multiplier", Params::kGroupDoF);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 1.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 2.0);
    }

    // --- 3D PARALLAX GROUP ---
    defineParam(kOfxParamTypeGroup, Params::kGroupParallax, "3D Parallax & 2.5D Displacement");
    {
        OfxPropertySetHandle p = defineParam(kOfxParamTypeBoolean, Params::kParamEnableParallax, "Enable 3D Parallax", Params::kGroupParallax);
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamParallaxX, "Horizontal Parallax", Params::kGroupParallax);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, -0.1);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 0.1);

        p = defineParam(kOfxParamTypeDouble, Params::kParamParallaxY, "Vertical Parallax", Params::kGroupParallax);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, -0.1);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 0.1);

        p = defineParam(kOfxParamTypeDouble, Params::kParamDepthStrength, "Displacement Strength", Params::kGroupParallax);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 1.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 3.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamCameraDistance, "Camera Perspective Distance", Params::kGroupParallax);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 2.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.5);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 10.0);

        p = defineParam(kOfxParamTypeChoice, Params::kParamEdgeFill, "Edge Fill Mode", Params::kGroupParallax);
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 0, "Mirror");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 1, "Blur Edge");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 2, "Repeat / Clamp");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 3, "Inpaint");
        gSuites.propSuite->propSetString(p, kOfxParamPropChoiceOption, 4, "Transparent");
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 0);
    }

    // --- DEPTH ZOOM GROUP ---
    defineParam(kOfxParamTypeGroup, Params::kGroupDepthZoom, "Depth Zoom Warp");
    {
        OfxPropertySetHandle p = defineParam(kOfxParamTypeBoolean, Params::kParamEnableDepthZoom, "Enable Depth Zoom", Params::kGroupDepthZoom);
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamNearScale, "Near Scale", Params::kGroupDepthZoom);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 1.1);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.5);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 2.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamFarScale, "Far Scale", Params::kGroupDepthZoom);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.95);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.5);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 2.0);

        p = defineParam(kOfxParamTypeDouble2D, Params::kParamZoomCenter, "Zoom Center (X, Y)", Params::kGroupDepthZoom);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.5);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 1, 0.5);
    }

    // --- DEPTH FOG GROUP ---
    defineParam(kOfxParamTypeGroup, Params::kGroupFog, "Atmospheric Depth Fog");
    {
        OfxPropertySetHandle p = defineParam(kOfxParamTypeBoolean, Params::kParamEnableFog, "Enable Depth Fog", Params::kGroupFog);
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamFogAmount, "Fog Density / Amount", Params::kGroupFog);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.5);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 1.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamFogStart, "Fog Start Distance", Params::kGroupFog);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.2);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 1.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamFogEnd, "Fog End Distance", Params::kGroupFog);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.9);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 1.0);

        p = defineParam(kOfxParamTypeRGBA, Params::kParamFogColor, "Fog Color", Params::kGroupFog);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.8);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 1, 0.85);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 2, 0.9);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 3, 1.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamFogFalloff, "Fog Falloff Curve", Params::kGroupFog);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 1.5);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.5);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 4.0);
    }

    // --- RELIGHTING GROUP ---
    defineParam(kOfxParamTypeGroup, Params::kGroupRelighting, "Scene Relighting (Normals from Depth)");
    {
        OfxPropertySetHandle p = defineParam(kOfxParamTypeBoolean, Params::kParamEnableRelighting, "Enable Relighting", Params::kGroupRelighting);
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamLightDirX, "Light Direction X", Params::kGroupRelighting);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.5);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, -1.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 1.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamLightDirY, "Light Direction Y", Params::kGroupRelighting);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, -0.5);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, -1.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 1.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamLightHeight, "Light Elevation / Height", Params::kGroupRelighting);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.8);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.1);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 2.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamLightStrength, "Light Intensity", Params::kGroupRelighting);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.8);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 3.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamLightSoftness, "Light Softness", Params::kGroupRelighting);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.5);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 1.0);

        p = defineParam(kOfxParamTypeDouble, Params::kParamAmbientLight, "Ambient Baseline Light", Params::kGroupRelighting);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.2);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 1.0);
    }

    // --- EDGE REFINEMENT GROUP ---
    defineParam(kOfxParamTypeGroup, Params::kGroupEdgeRefine, "Edge Refinement (Guided Filter)");
    {
        OfxPropertySetHandle p = defineParam(kOfxParamTypeBoolean, Params::kParamEnableEdgeRefine, "Enable Edge Refinement", Params::kGroupEdgeRefine);
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 1);

        p = defineParam(kOfxParamTypeInteger, Params::kParamEdgeRadius, "Refinement Radius", Params::kGroupEdgeRefine);
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 4);
        gSuites.propSuite->propSetInt(p, kOfxParamPropDisplayMin, 0, 1);
        gSuites.propSuite->propSetInt(p, kOfxParamPropDisplayMax, 0, 16);

        p = defineParam(kOfxParamTypeDouble, Params::kParamEdgeEps, "Refinement Epsilon", Params::kGroupEdgeRefine);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDefault, 0, 0.001);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMin, 0, 0.0001);
        gSuites.propSuite->propSetDouble(p, kOfxParamPropDisplayMax, 0, 0.05);
    }

    // --- DEBUG & PERFORMANCE GROUP ---
    defineParam(kOfxParamTypeGroup, Params::kGroupDebug, "Performance & Diagnostics");
    {
        OfxPropertySetHandle p = defineParam(kOfxParamTypeBoolean, Params::kParamShowPerfOverlay, "Show On-Screen Performance HUD", Params::kGroupDebug);
        gSuites.propSuite->propSetInt(p, kOfxParamPropDefault, 0, 0);

        defineParam(kOfxParamTypePushButton, Params::kParamClearCache, "Clear Depth Cache (RAM + Disk)", Params::kGroupDebug);
    }

    return kOfxStatOK;
}

static OfxStatus pluginMainEntry(const char* action, const void* handle, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs) {
    if (!action) return kOfxStatFailed;

    if (std::strcmp(action, kOfxActionLoad) == 0) {
        if (!gSuites.propSuite || !gSuites.paramSuite || !gSuites.effectSuite) return kOfxStatErrMissingHostFeature;
        LOG_INFO("AI Depth Pro: Plugin loaded by host.");
        return kOfxStatOK;
    }

    if (std::strcmp(action, kOfxActionUnload) == 0) {
        LOG_INFO("AI Depth Pro: Plugin unloaded by host.");
        return kOfxStatOK;
    }

    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        return describePlugin((OfxImageEffectHandle)handle);
    }

    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        return describePluginInContext((OfxImageEffectHandle)handle, inArgs);
    }

    if (std::strcmp(action, kOfxActionCreateInstance) == 0) {
        PluginInstance* instance = new PluginInstance((OfxImageEffectHandle)handle, gSuites);
        OfxPropertySetHandle effectProps = nullptr;
        gSuites.effectSuite->getPropertySet((OfxImageEffectHandle)handle, &effectProps);
        if (gSuites.propSuite->propSetPointer(effectProps, kOfxPropInstanceData, 0, (void*)instance) != kOfxStatOK) { delete instance; return kOfxStatFailed; }
        return kOfxStatOK;
    }

    if (std::strcmp(action, kOfxActionDestroyInstance) == 0) {
        OfxPropertySetHandle effectProps = nullptr;
        gSuites.effectSuite->getPropertySet((OfxImageEffectHandle)handle, &effectProps);
        void* ptr = nullptr;
        gSuites.propSuite->propGetPointer(effectProps, kOfxPropInstanceData, 0, &ptr);
        if (ptr) {
            PluginInstance* instance = static_cast<PluginInstance*>(ptr);
            delete instance;
            gSuites.propSuite->propSetPointer(effectProps, kOfxPropInstanceData, 0, nullptr);
        }
        return kOfxStatOK;
    }

    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        OfxPropertySetHandle effectProps = nullptr;
        gSuites.effectSuite->getPropertySet((OfxImageEffectHandle)handle, &effectProps);
        void* ptr = nullptr;
        gSuites.propSuite->propGetPointer(effectProps, kOfxPropInstanceData, 0, &ptr);
        if (ptr) {
            PluginInstance* instance = static_cast<PluginInstance*>(ptr);
            return instance->Render(inArgs, outArgs);
        }
        return kOfxStatErrBadHandle;
    }

    if (std::strcmp(action, kOfxActionPurge) == 0) {
        OfxPropertySetHandle effectProps = nullptr;
        gSuites.effectSuite->getPropertySet((OfxImageEffectHandle)handle, &effectProps);
        void* ptr = nullptr;
        gSuites.propSuite->propGetPointer(effectProps, kOfxPropInstanceData, 0, &ptr);
        if (ptr) {
            PluginInstance* instance = static_cast<PluginInstance*>(ptr);
            return instance->Purge();
        }
        return kOfxStatOK;
    }

    if (std::strcmp(action, kOfxActionSync) == 0) {
        return kOfxStatOK;
    }

    if (std::strcmp(action, kOfxActionInstanceChanged) == 0) {
        OfxPropertySetHandle effectProps = nullptr;
        gSuites.effectSuite->getPropertySet((OfxImageEffectHandle)handle, &effectProps);
        void* ptr = nullptr;
        gSuites.propSuite->propGetPointer(effectProps, kOfxPropInstanceData, 0, &ptr);
        if (ptr) {
            PluginInstance* instance = static_cast<PluginInstance*>(ptr);
            return instance->OnInstanceChanged(inArgs);
        }
        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

static OfxStatus safeMainEntry(const char* action, const void* handle, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs) {
    try { return pluginMainEntry(action, handle, inArgs, outArgs); }
    catch (const std::bad_alloc&) { return kOfxStatErrMemory; }
    catch (...) { return kOfxStatFailed; }
}

static void setHost(OfxHost* host) {
    gHost = host;
    if (!gHost) return;

    gSuites.propSuite = (OfxPropertySuiteV1*)gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1);
    gSuites.paramSuite = (OfxParameterSuiteV1*)gHost->fetchSuite(gHost->host, kOfxParameterSuite, 1);
    gSuites.effectSuite = (OfxImageEffectSuiteV1*)gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1);
    gSuites.threadSuite = (OfxMultiThreadSuiteV1*)gHost->fetchSuite(gHost->host, kOfxMultiThreadSuite, 1);
    gSuites.messageSuite = (OfxMessageSuiteV1*)gHost->fetchSuite(gHost->host, kOfxMessageSuite, 1);
    gSuites.memorySuite = (OfxMemorySuiteV1*)gHost->fetchSuite(gHost->host, kOfxMemorySuite, 1);
}

static OfxPlugin gPluginDefinition = {
    kOfxImageEffectPluginApi,
    1,
    "com.aidepthpro.depthgenerator",
    1,
    0,
    setHost,
    safeMainEntry
};

#ifdef __cplusplus
extern "C" {
#endif

OfxExport OfxPlugin* OfxGetPlugin(int nth) {
    if (nth == 0) return &gPluginDefinition;
    return nullptr;
}

OfxExport int OfxGetNumberOfPlugins(void) {
    return 1;
}

#ifdef __cplusplus
}
#endif
