#ifndef _ofxMultiThread_h_
#define _ofxMultiThread_h_

#include "ofxCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kOfxMultiThreadSuite "OfxMultiThreadSuite"

typedef void (*OfxThreadFunctionV1)(unsigned int threadIndex, unsigned int threadMax, void* customArg);

typedef void* OfxMutexHandle;

typedef struct OfxMultiThreadSuiteV1 {
  OfxStatus (*multiThread)(OfxThreadFunctionV1 func, unsigned int nThreads, void* customArg);
  OfxStatus (*multiThreadNumCPUs)(unsigned int* nCPUs);
  OfxStatus (*multiThreadIndex)(unsigned int* threadIndex);
  OfxStatus (*multiThreadIsSpawnedThread)(int* isSpawned);
  OfxStatus (*mutexCreate)(OfxMutexHandle* mutex, int lockCount);
  OfxStatus (*mutexDestroy)(OfxMutexHandle mutex);
  OfxStatus (*mutexLock)(OfxMutexHandle mutex);
  OfxStatus (*mutexUnLock)(OfxMutexHandle mutex);
  OfxStatus (*mutexTryLock)(OfxMutexHandle mutex);
} OfxMultiThreadSuiteV1;

#ifdef __cplusplus
}
#endif

#endif
