#ifndef _ofxImageEffect_h_
#define _ofxImageEffect_h_

#include "ofxCore.h"
#include "ofxProperty.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kOfxImageEffectSuite "OfxImageEffectSuite"

#define kOfxTypeImageEffect "OfxTypeImageEffect"
#define kOfxTypeImageEffectHost "OfxTypeImageEffectHost"

#define kOfxImageEffectActionDescribeInContext "OfxImageEffectActionDescribeInContext"
#define kOfxImageEffectActionGetRegionOfDefinition "OfxImageEffectActionGetRegionOfDefinition"
#define kOfxImageEffectActionGetRegionsOfInterest "OfxImageEffectActionGetRegionsOfInterest"
#define kOfxImageEffectActionGetTimeDomain "OfxImageEffectActionGetTimeDomain"
#define kOfxImageEffectActionGetFramesNeeded "OfxImageEffectActionGetFramesNeeded"
#define kOfxImageEffectActionGetClipPreferences "OfxImageEffectActionGetClipPreferences"
#define kOfxImageEffectActionIsIdentity "OfxImageEffectActionIsIdentity"
#define kOfxImageEffectActionRender "OfxImageEffectActionRender"
#define kOfxImageEffectActionBeginSequenceRender "OfxImageEffectActionBeginSequenceRender"
#define kOfxImageEffectActionEndSequenceRender "OfxImageEffectActionEndSequenceRender"

#define kOfxImageEffectContextFilter "OfxImageEffectContextFilter"
#define kOfxImageEffectContextGeneral "OfxImageEffectContextGeneral"
#define kOfxImageEffectContextGenerator "OfxImageEffectContextGenerator"
#define kOfxImageEffectContextTransition "OfxImageEffectContextTransition"
#define kOfxImageEffectContextPaint "OfxImageEffectContextPaint"
#define kOfxImageEffectContextRetimer "OfxImageEffectContextRetimer"

#define kOfxBitDepthByte "OfxBitDepthByte"
#define kOfxBitDepthShort "OfxBitDepthShort"
#define kOfxBitDepthHalf "OfxBitDepthHalf"
#define kOfxBitDepthFloat "OfxBitDepthFloat"
#define kOfxBitDepthNone "OfxBitDepthNone"

#define kOfxImageComponentRGBA "OfxImageComponentRGBA"
#define kOfxImageComponentRGB "OfxImageComponentRGB"
#define kOfxImageComponentAlpha "OfxImageComponentAlpha"
#define kOfxImageComponentNone "OfxImageComponentNone"

#define kOfxImageFieldNone "OfxImageFieldNone"
#define kOfxImageFieldBoth "OfxImageFieldBoth"
#define kOfxImageFieldLower "OfxImageFieldLower"
#define kOfxImageFieldUpper "OfxImageFieldUpper"

#define kOfxImageEffectPluginPropSingleInstance "OfxImageEffectPluginPropSingleInstance"
#define kOfxImageEffectPluginPropHostFrameThreading "OfxImageEffectPluginPropHostFrameThreading"
#define kOfxImageEffectPluginRenderThreadSafety "OfxImageEffectPluginRenderThreadSafety"
#define kOfxImageEffectRenderFullySafe "OfxImageEffectRenderFullySafe"
#define kOfxImageEffectRenderInstanceSafe "OfxImageEffectRenderInstanceSafe"
#define kOfxImageEffectRenderUnsafe "OfxImageEffectRenderUnsafe"

#define kOfxImageEffectPropSupportedContexts "OfxImageEffectPropSupportedContexts"
#define kOfxImageEffectPropSupportedPixelDepths "OfxImageEffectPropSupportedPixelDepths"
#define kOfxImageEffectPropSupportedComponents "OfxImageEffectPropSupportedComponents"
#define kOfxImageEffectPropContext "OfxImageEffectPropContext"
#define kOfxImageEffectPropPixelDepth "OfxImageEffectPropPixelDepth"
#define kOfxImageEffectPropComponents "OfxImageEffectPropComponents"
#define kOfxImageEffectPropProjectSize "OfxImageEffectPropProjectSize"
#define kOfxImageEffectPropProjectOffset "OfxImageEffectPropProjectOffset"
#define kOfxImageEffectPropProjectExtent "OfxImageEffectPropProjectExtent"
#define kOfxImageEffectPropProjectPixelAspectRatio "OfxImageEffectPropProjectPixelAspectRatio"
#define kOfxImageEffectPropFrameRate "OfxImageEffectPropFrameRate"
#define kOfxImageEffectPropUnmappedFrameRate "OfxImageEffectPropUnmappedFrameRate"
#define kOfxImageEffectPropRenderWindow "OfxImageEffectPropRenderWindow"
#define kOfxImageEffectPropRenderScale "OfxImageEffectPropRenderScale"
#define kOfxImageEffectPropRegionOfDefinition "OfxImageEffectPropRegionOfDefinition"
#define kOfxImageEffectPropRegionOfInterest "OfxImageEffectPropRegionOfInterest"
#define kOfxImageEffectPropSequentialRender "OfxImageEffectPropSequentialRender"
#define kOfxImageEffectPropInteractiveRender "OfxImageEffectPropInteractiveRender"
#define kOfxImageEffectPropOpenGLEnabled "OfxImageEffectPropOpenGLEnabled"
#define kOfxImageEffectPropCudaEnabled "OfxImageEffectPropCudaEnabled"
#define kOfxImageEffectPropMetalEnabled "OfxImageEffectPropMetalEnabled"
#define kOfxImageEffectPropOpenCLEnabled "OfxImageEffectPropOpenCLEnabled"
#define kOfxImageEffectPluginPropGrouping "OfxImageEffectPluginPropGrouping"

#define kOfxImageClipPropRowBytes "OfxImageClipPropRowBytes"
#define kOfxImageClipPropContinuousSamples "OfxImageClipPropContinuousSamples"
#define kOfxImageClipPropUnmappedComponents "OfxImageClipPropUnmappedComponents"
#define kOfxImageClipPropUnmappedBitDepth "OfxImageClipPropUnmappedBitDepth"
#define kOfxImageClipPropOptional "OfxImageClipPropOptional"
#define kOfxImageClipPropIsOutput "OfxImageClipPropIsOutput"
#define kOfxImageClipPropFieldOrder "OfxImageClipPropFieldOrder"

#define kOfxImagePropData "OfxImagePropData"
#define kOfxImagePropRowBytes "OfxImagePropRowBytes"
#define kOfxImagePropBounds "OfxImagePropBounds"
#define kOfxImagePropRegionOfDefinition "OfxImagePropRegionOfDefinition"
#define kOfxImagePropPixelAspectRatio "OfxImagePropPixelAspectRatio"
#define kOfxImagePropUniqueIdentifier "OfxImagePropUniqueIdentifier"
#define kOfxImagePropRenderScale "OfxImagePropRenderScale"

#define kOfxImageEffectOutputClipName "Output"
#define kOfxImageEffectSimpleSourceClipName "Source"

typedef struct OfxImageEffectSuiteV1 {
  OfxStatus (*getPropertySet)(OfxImageEffectHandle imageEffect, OfxPropertySetHandle* propHandle);
  OfxStatus (*getParamSet)(OfxImageEffectHandle imageEffect, OfxParamSetHandle* paramSet);
  OfxStatus (*clipDefine)(OfxImageEffectHandle imageEffect, const char* name, OfxPropertySetHandle* propertySet);
  OfxStatus (*clipGetHandle)(OfxImageEffectHandle imageEffect, const char* name, OfxImageClipHandle* clip, OfxPropertySetHandle* propertySet);
  OfxStatus (*clipGetPropertySet)(OfxImageClipHandle clip, OfxPropertySetHandle* propHandle);
  OfxStatus (*clipGetImage)(OfxImageClipHandle clip, OfxTime time, const OfxRectD* region, OfxPropertySetHandle* imageHandle);
  OfxStatus (*clipReleaseImage)(OfxPropertySetHandle imageHandle);
  OfxStatus (*clipGetRegionOfDefinition)(OfxImageClipHandle clip, OfxTime time, OfxRectD* bounds);
  OfxStatus (*abort)(OfxImageEffectHandle imageEffect);
  OfxStatus (*imageMemoryAlloc)(OfxImageEffectHandle imageEffect, size_t nBytes, OfxImageMemoryHandle* memoryHandle);
  OfxStatus (*imageMemoryFree)(OfxImageMemoryHandle memoryHandle);
  OfxStatus (*imageMemoryLock)(OfxImageMemoryHandle memoryHandle, void** returnedPtr);
  OfxStatus (*imageMemoryUnlock)(OfxImageMemoryHandle memoryHandle);
} OfxImageEffectSuiteV1;

#ifdef __cplusplus
}
#endif

#endif
