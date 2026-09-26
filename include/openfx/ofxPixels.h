#ifndef _ofxPixels_h_
#define _ofxPixels_h_

#include "ofxCore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OfxRGBAColourB_t {
  unsigned char r, g, b, a;
} OfxRGBAColourB_t;

typedef struct OfxRGBAColourS_t {
  unsigned short r, g, b, a;
} OfxRGBAColourS_t;

typedef struct OfxRGBAColourF_t {
  float r, g, b, a;
} OfxRGBAColourF_t;

typedef struct OfxRGBAColourD_t {
  double r, g, b, a;
} OfxRGBAColourD_t;

#ifdef __cplusplus
}
#endif

#endif
