#ifndef _ofxMessage_h_
#define _ofxMessage_h_

#include "ofxCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kOfxMessageSuite "OfxMessageSuite"

#define kOfxMessageFatal "OfxMessageFatal"
#define kOfxMessageError "OfxMessageError"
#define kOfxMessageWarning "OfxMessageWarning"
#define kOfxMessageMessage "OfxMessageMessage"
#define kOfxMessageLog "OfxMessageLog"
#define kOfxMessageQuestion "OfxMessageQuestion"

typedef struct OfxMessageSuiteV1 {
  OfxStatus (*message)(void* handle, const char* messageType, const char* messageId, const char* format, ...);
} OfxMessageSuiteV1;

#ifdef __cplusplus
}
#endif

#endif
