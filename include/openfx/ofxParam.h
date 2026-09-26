#ifndef _ofxParam_h_
#define _ofxParam_h_

#include "ofxCore.h"
#include "ofxProperty.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kOfxParameterSuite "OfxParameterSuite"

#define kOfxParamTypeInteger "OfxParamTypeInteger"
#define kOfxParamTypeDouble "OfxParamTypeDouble"
#define kOfxParamTypeBoolean "OfxParamTypeBoolean"
#define kOfxParamTypeChoice "OfxParamTypeChoice"
#define kOfxParamTypeRGBA "OfxParamTypeRGBA"
#define kOfxParamTypeRGB "OfxParamTypeRGB"
#define kOfxParamTypeDouble2D "OfxParamTypeDouble2D"
#define kOfxParamTypeInteger2D "OfxParamTypeInteger2D"
#define kOfxParamTypeDouble3D "OfxParamTypeDouble3D"
#define kOfxParamTypeInteger3D "OfxParamTypeInteger3D"
#define kOfxParamTypeString "OfxParamTypeString"
#define kOfxParamTypeCustom "OfxParamTypeCustom"
#define kOfxParamTypeGroup "OfxParamTypeGroup"
#define kOfxParamTypePage "OfxParamTypePage"
#define kOfxParamTypePushButton "OfxParamTypePushButton"

#define kOfxParamPropType "OfxParamPropType"
#define kOfxParamPropAnimates "OfxParamPropAnimates"
#define kOfxParamPropIsAutoCreated "OfxParamPropIsAutoCreated"
#define kOfxParamPropPluginMayProvideGrey "OfxParamPropPluginMayProvideGrey"
#define kOfxParamPropPersist "OfxParamPropPersist"
#define kOfxParamPropSecret "OfxParamPropSecret"
#define kOfxParamPropCacheInvalidation "OfxParamPropCacheInvalidation"
#define kOfxParamPropDefault "OfxParamPropDefault"
#define kOfxParamPropDisplayMin "OfxParamPropDisplayMin"
#define kOfxParamPropDisplayMax "OfxParamPropDisplayMax"
#define kOfxParamPropMin "OfxParamPropMin"
#define kOfxParamPropMax "OfxParamPropMax"
#define kOfxParamPropDigits "OfxParamPropDigits"
#define kOfxParamPropIncrement "OfxParamPropIncrement"
#define kOfxParamPropParent "OfxParamPropParent"
#define kOfxParamPropChoiceOption "OfxParamPropChoiceOption"
#define kOfxParamPropPageChild "OfxParamPropPageChild"
#define kOfxParamPropStringMode "OfxParamPropStringMode"
#define kOfxParamPropStringFilePath "OfxParamPropStringFilePath"
#define kOfxParamPropStringDirectoryPath "OfxParamPropStringDirectoryPath"
#define kOfxParamPropStringLabel "OfxParamPropStringLabel"
#define kOfxParamPropScriptName "OfxParamPropScriptName"
#define kOfxParamPropDoubleType "OfxParamPropDoubleType"
#define kOfxParamDoubleTypePlain "OfxParamDoubleTypePlain"
#define kOfxParamDoubleTypeAngle "OfxParamDoubleTypeAngle"
#define kOfxParamDoubleTypeScale "OfxParamDoubleTypeScale"
#define kOfxParamDoubleTypeTime "OfxParamDoubleTypeTime"
#define kOfxParamDoubleTypeAbsoluteTime "OfxParamDoubleTypeAbsoluteTime"
#define kOfxParamDoubleTypeNormalisedX "OfxParamDoubleTypeNormalisedX"
#define kOfxParamDoubleTypeNormalisedY "OfxParamDoubleTypeNormalisedY"
#define kOfxParamDoubleTypeNormalisedXY "OfxParamDoubleTypeNormalisedXY"
#define kOfxParamDoubleTypeNormalisedXAbsolute "OfxParamDoubleTypeNormalisedXAbsolute"
#define kOfxParamDoubleTypeNormalisedYAbsolute "OfxParamDoubleTypeNormalisedYAbsolute"
#define kOfxParamDoubleTypeNormalisedXYAbsolute "OfxParamDoubleTypeNormalisedXYAbsolute"
#define kOfxParamPropCustomValue "OfxParamPropCustomValue"

#define kOfxParamPropEnabled "OfxParamPropEnabled"
#define kOfxParamPropEvaluateOnChange "OfxParamPropEvaluateOnChange"

typedef struct OfxParameterSuiteV1 {
  OfxStatus (*paramDefine)(OfxParamSetHandle paramSet, const char* paramType, const char* name, OfxPropertySetHandle* propertySet);
  OfxStatus (*paramGetHandle)(OfxParamSetHandle paramSet, const char* name, OfxParamHandle* param, OfxPropertySetHandle* propertySet);
  OfxStatus (*paramSetGetPropertySet)(OfxParamSetHandle paramSet, OfxPropertySetHandle* propHandle);
  OfxStatus (*paramGetPropertySet)(OfxParamHandle param, OfxPropertySetHandle* propHandle);
  
  OfxStatus (*paramGetValue)(OfxParamHandle paramHandle, ...);
  OfxStatus (*paramGetValueAtTime)(OfxParamHandle paramHandle, OfxTime time, ...);
  OfxStatus (*paramGetDerivative)(OfxParamHandle paramHandle, OfxTime time, ...);
  OfxStatus (*paramGetIntegral)(OfxParamHandle paramHandle, OfxTime time1, OfxTime time2, ...);
  
  OfxStatus (*paramSetValue)(OfxParamHandle paramHandle, ...);
  OfxStatus (*paramSetValueAtTime)(OfxParamHandle paramHandle, OfxTime time, ...);
  OfxStatus (*paramGetNumKeys)(OfxParamHandle paramHandle, unsigned int* numberOfKeys);
  OfxStatus (*paramGetKeyTime)(OfxParamHandle paramHandle, unsigned int nthKey, OfxTime* time);
  OfxStatus (*paramGetKeyIndex)(OfxParamHandle paramHandle, OfxTime time, int direction, int* index);
  OfxStatus (*paramDeleteKey)(OfxParamHandle paramHandle, OfxTime time);
  OfxStatus (*paramDeleteAllKeys)(OfxParamHandle paramHandle);
  OfxStatus (*paramCopy)(OfxParamHandle paramTo, OfxParamHandle paramFrom, OfxTime dstOffset, const OfxRangeD* frameRange);
  OfxStatus (*paramEditBegin)(OfxParamSetHandle paramSet, const char* name);
  OfxStatus (*paramEditEnd)(OfxParamSetHandle paramSet);
} OfxParameterSuiteV1;

#ifdef __cplusplus
}
#endif

#endif
