#ifndef _ofxProgress_h_
#define _ofxProgress_h_

#include "ofxCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kOfxProgressSuite "OfxProgressSuite"

typedef struct OfxProgressSuiteV1 {
  OfxStatus (*progressStart)(void* effectInstance, const char* message);
  OfxStatus (*progressUpdate)(void* effectInstance, double progress);
  OfxStatus (*progressEnd)(void* effectInstance);
} OfxProgressSuiteV1;

#ifdef __cplusplus
}
#endif

#endif
