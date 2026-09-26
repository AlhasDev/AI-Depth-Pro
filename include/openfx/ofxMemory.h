#ifndef _ofxMemory_h_
#define _ofxMemory_h_

#include "ofxCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kOfxMemorySuite "OfxMemorySuite"

typedef struct OfxMemorySuiteV1 {
  OfxStatus (*memoryAlloc)(void* handle, size_t nBytes, void** allocatedData);
  OfxStatus (*memoryFree)(void* allocatedData);
} OfxMemorySuiteV1;

#ifdef __cplusplus
}
#endif

#endif
