#ifndef _ofxMetalRender_h_
#define _ofxMetalRender_h_

#include "ofxCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kOfxMetalRenderSuite "OfxMetalRenderSuite"

#define kOfxImageEffectPropMetalEnabled "OfxImageEffectPropMetalEnabled"
#define kOfxImageEffectPropMetalCommandQueue "OfxImageEffectPropMetalCommandQueue"
#define kOfxImagePropMetalCommandQueue "OfxImagePropMetalCommandQueue"

typedef struct OfxMetalRenderSuiteV1 {
  OfxStatus (*metalCommandQueueGet)(OfxImageEffectHandle effect, void** commandQueue);
} OfxMetalRenderSuiteV1;

#ifdef __cplusplus
}
#endif

#endif
