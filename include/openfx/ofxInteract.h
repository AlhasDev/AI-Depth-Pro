#ifndef _ofxInteract_h_
#define _ofxInteract_h_

#include "ofxCore.h"
#include "ofxProperty.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kOfxInteractSuite "OfxInteractSuite"

#define kOfxInteractActionDescribe "OfxInteractActionDescribe"
#define kOfxInteractActionCreateInstance "OfxInteractActionCreateInstance"
#define kOfxInteractActionDestroyInstance "OfxInteractActionDestroyInstance"
#define kOfxInteractActionDraw "OfxInteractActionDraw"
#define kOfxInteractActionPenMotion "OfxInteractActionPenMotion"
#define kOfxInteractActionPenDown "OfxInteractActionPenDown"
#define kOfxInteractActionPenUp "OfxInteractActionPenUp"
#define kOfxInteractActionKeyDown "OfxInteractActionKeyDown"
#define kOfxInteractActionKeyUp "OfxInteractActionKeyUp"
#define kOfxInteractActionKeyRepeat "OfxInteractActionKeyRepeat"
#define kOfxInteractActionGainFocus "OfxInteractActionGainFocus"
#define kOfxInteractActionLoseFocus "OfxInteractActionLoseFocus"

#define kOfxInteractPropPixelScale "OfxInteractPropPixelScale"
#define kOfxInteractPropBackgroundColour "OfxInteractPropBackgroundColour"
#define kOfxInteractPropPenPosition "OfxInteractPropPenPosition"
#define kOfxInteractPropPenViewportPosition "OfxInteractPropPenViewportPosition"
#define kOfxInteractPropPenPressure "OfxInteractPropPenPressure"
#define kOfxInteractPropKeyString "OfxInteractPropKeyString"
#define kOfxInteractPropKeySym "OfxInteractPropKeySym"
#define kOfxInteractPropBitDepth "OfxInteractPropBitDepth"
#define kOfxInteractPropHasAlpha "OfxInteractPropHasAlpha"
#define kOfxInteractPropEffectInstance "OfxInteractPropEffectInstance"

typedef struct OfxInteractSuiteV1 {
  OfxStatus (*interactSwapBuffers)(OfxInteractHandle interactInstance);
  OfxStatus (*interactRedraw)(OfxInteractHandle interactInstance);
  OfxStatus (*interactGetPropertySet)(OfxInteractHandle interactInstance, OfxPropertySetHandle* property);
} OfxInteractSuiteV1;

#ifdef __cplusplus
}
#endif

#endif
