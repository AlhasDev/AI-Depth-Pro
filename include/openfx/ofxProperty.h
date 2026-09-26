#ifndef _ofxProperty_h_
#define _ofxProperty_h_

#include "ofxCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kOfxPropertySuite "OfxPropertySuite"

#define kOfxPropType "OfxPropType"
#define kOfxPropName "OfxPropName"
#define kOfxPropLabel "OfxPropLabel"
#define kOfxPropShortLabel "OfxPropShortLabel"
#define kOfxPropLongLabel "OfxPropLongLabel"
#define kOfxPropIcon "OfxPropIcon"
#define kOfxPropInstanceData "OfxPropInstanceData"
#define kOfxPropPluginDescription "OfxPropPluginDescription"
#define kOfxPropTypePointer "OfxPropTypePointer"
#define kOfxPropTypeString "OfxPropTypeString"
#define kOfxPropTypeDouble "OfxPropTypeDouble"
#define kOfxPropTypeInt "OfxPropTypeInt"
#define kOfxPropDimension "OfxPropDimension"
#define kOfxPropIsReadOnly "OfxPropIsReadOnly"
#define kOfxPropHostOS "OfxPropHostOS"
#define kOfxPropAPIVersion "OfxPropAPIVersion"
#define kOfxPropVersion "OfxPropVersion"
#define kOfxPropVersionLabel "OfxPropVersionLabel"

typedef struct OfxPropertySuiteV1 {
  OfxStatus (*propSetPointer)(OfxPropertySetHandle properties, const char* property, int index, void* value);
  OfxStatus (*propSetString)(OfxPropertySetHandle properties, const char* property, int index, const char* value);
  OfxStatus (*propSetDouble)(OfxPropertySetHandle properties, const char* property, int index, double value);
  OfxStatus (*propSetInt)(OfxPropertySetHandle properties, const char* property, int index, int value);
  OfxStatus (*propSetPointerN)(OfxPropertySetHandle properties, const char* property, int count, void* const* value);
  OfxStatus (*propSetStringN)(OfxPropertySetHandle properties, const char* property, int count, const char* const* value);
  OfxStatus (*propSetDoubleN)(OfxPropertySetHandle properties, const char* property, int count, const double* value);
  OfxStatus (*propSetIntN)(OfxPropertySetHandle properties, const char* property, int count, const int* value);

  OfxStatus (*propGetPointer)(OfxPropertySetHandle properties, const char* property, int index, void** value);
  OfxStatus (*propGetString)(OfxPropertySetHandle properties, const char* property, int index, char** value);
  OfxStatus (*propGetDouble)(OfxPropertySetHandle properties, const char* property, int index, double* value);
  OfxStatus (*propGetInt)(OfxPropertySetHandle properties, const char* property, int index, int* value);
  OfxStatus (*propGetPointerN)(OfxPropertySetHandle properties, const char* property, int count, void** value);
  OfxStatus (*propGetStringN)(OfxPropertySetHandle properties, const char* property, int count, char** value);
  OfxStatus (*propGetDoubleN)(OfxPropertySetHandle properties, const char* property, int count, double* value);
  OfxStatus (*propGetIntN)(OfxPropertySetHandle properties, const char* property, int count, int* value);

  OfxStatus (*propReset)(OfxPropertySetHandle properties, const char* property);
  OfxStatus (*propGetDimension)(OfxPropertySetHandle properties, const char* property, int* count);
} OfxPropertySuiteV1;

#ifdef __cplusplus
}
#endif

#endif
