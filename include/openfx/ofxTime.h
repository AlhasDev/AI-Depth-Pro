#ifndef _ofxTime_h_
#define _ofxTime_h_

#include "ofxCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kOfxTimeSuite "OfxTimeSuite"

typedef struct OfxTimeSuiteV1 {
  OfxStatus (*getTimelineInfo)(void* effectInstance, double* timelineFps, double* timelineTime);
} OfxTimeSuiteV1;

#ifdef __cplusplus
}
#endif

#endif
