#ifndef _ofxCore_h_
#define _ofxCore_h_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
  #define OfxExport __declspec(dllexport)
  #define OfxImport __declspec(dllimport)
#else
  #define OfxExport __attribute__((visibility("default")))
  #define OfxImport
#endif

#define kOfxTypePropertySet "OfxTypePropertySet"
#define kOfxTypeClip "OfxTypeClip"
#define kOfxTypeImage "OfxTypeImage"
#define kOfxTypeParameter "OfxTypeParameter"
#define kOfxTypeParameterInstance "OfxTypeParameterInstance"

typedef int OfxStatus;
#define kOfxStatOK 0
#define kOfxStatFailed 1
#define kOfxStatErrFatal 2
#define kOfxStatErrUnknown 3
#define kOfxStatErrMissingHostFeature 4
#define kOfxStatErrUnsupported 5
#define kOfxStatErrExists 6
#define kOfxStatErrFormat 7
#define kOfxStatErrMemory 8
#define kOfxStatErrBadHandle 9
#define kOfxStatErrBadIndex 10
#define kOfxStatErrValue 11
#define kOfxStatReplyYes 12
#define kOfxStatReplyNo 13
#define kOfxStatReplyDefault 14

typedef double OfxTime;

typedef struct OfxPointD {
  double x, y;
} OfxPointD;

typedef struct OfxPointI {
  int x, y;
} OfxPointI;

typedef struct OfxRangeD {
  double min, max;
} OfxRangeD;

typedef struct OfxRangeI {
  int min, max;
} OfxRangeI;

typedef struct OfxRectD {
  double x1, y1, x2, y2;
} OfxRectD;

typedef struct OfxRectI {
  int x1, y1, x2, y2;
} OfxRectI;

typedef struct OfxRGBAColourB {
  unsigned char r, g, b, a;
} OfxRGBAColourB;

typedef struct OfxRGBAColourS {
  unsigned short r, g, b, a;
} OfxRGBAColourS;

typedef struct OfxRGBAColourF {
  float r, g, b, a;
} OfxRGBAColourF;

typedef struct OfxRGBAColourD {
  double r, g, b, a;
} OfxRGBAColourD;

typedef struct OfxRGBColourB {
  unsigned char r, g, b;
} OfxRGBColourB;

typedef struct OfxRGBColourS {
  unsigned short r, g, b;
} OfxRGBColourS;

typedef struct OfxRGBColourF {
  float r, g, b;
} OfxRGBColourF;

typedef struct OfxRGBColourD {
  double r, g, b;
} OfxRGBColourD;

typedef void* OfxPropertySetHandle;
typedef void* OfxImageEffectHandle;
typedef void* OfxImageClipHandle;
typedef void* OfxImageMemoryHandle;
typedef void* OfxParamHandle;
typedef void* OfxParamSetHandle;
typedef void* OfxInteractHandle;

typedef struct OfxHost {
  OfxPropertySetHandle host;
  void* (*fetchSuite)(OfxPropertySetHandle host, const char* suiteName, int suiteVersion);
} OfxHost;

typedef struct OfxPlugin {
  const char* pluginApi;
  int apiVersion;
  const char* pluginIdentifier;
  int pluginVersionMajor;
  int pluginVersionMinor;
  void (*setHost)(OfxHost* host);
  OfxStatus (*mainEntry)(const char* action, const void* handle, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs);
} OfxPlugin;

typedef OfxPlugin* (*OfxGetPluginFunc)(int nth);
typedef int (*OfxGetNumberOfPluginsFunc)(void);

#define kOfxActionLoad "OfxActionLoad"
#define kOfxActionDescribe "OfxActionDescribe"
#define kOfxActionUnload "OfxActionUnload"
#define kOfxActionPurge "OfxActionPurge"
#define kOfxActionSync "OfxActionSync"
#define kOfxActionCreateInstance "OfxActionCreateInstance"
#define kOfxActionDestroyInstance "OfxActionDestroyInstance"
#define kOfxActionInstanceChanged "OfxActionInstanceChanged"
#define kOfxActionBeginInstanceChanged "OfxActionBeginInstanceChanged"
#define kOfxActionEndInstanceChanged "OfxActionEndInstanceChanged"
#define kOfxActionBeginInstanceEdit "OfxActionBeginInstanceEdit"
#define kOfxActionEndInstanceEdit "OfxActionEndInstanceEdit"

#define kOfxImageEffectPluginApi "OfxImageEffectPluginAPI"
#define kOfxImageEffectPluginAPI "OfxImageEffectPluginAPI"
#define kOfxPropTime "OfxPropTime"

#ifdef __cplusplus
}
#endif

#endif
