#ifndef _ofxCudaRender_h_
#define _ofxCudaRender_h_

#include "ofxCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kOfxCudaRenderSuite "OfxCudaRenderSuite"

#define kOfxImageEffectPropCudaEnabled "OfxImageEffectPropCudaEnabled"
#define kOfxImageEffectPropCudaStream "OfxImageEffectPropCudaStream"
#define kOfxImagePropCudaStream "OfxImagePropCudaStream"

typedef struct OfxCudaRenderSuiteV1 {
  OfxStatus (*cudaStreamGet)(OfxImageEffectHandle effect, void** stream);
} OfxCudaRenderSuiteV1;

#ifdef __cplusplus
}
#endif

#endif
