#include "Plugin.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <vector>
extern "C" OfxPlugin* OfxGetPlugin(int);
struct Props {
    void* data=nullptr; void* instance=nullptr;
    int bounds[4]{10,20,18,24}, window[4]{10,20,18,24};
    int stride=0;
    double time=0;
    std::string name,depth=kOfxBitDepthFloat,components=kOfxImageComponentRGBA;
} effect,sourceImage,destImage,args;
static int released=0;
OfxStatus setPtr(OfxPropertySetHandle h,const char* key,int,void* p) {
    if(std::strcmp(key,kOfxPropInstanceData)) return kOfxStatErrValue;
    static_cast<Props*>(h)->instance=p; return kOfxStatOK;
}
OfxStatus getPtr(OfxPropertySetHandle h,const char* key,int,void** p) {
    auto* v=static_cast<Props*>(h);
    if(!std::strcmp(key,kOfxPropInstanceData)) *p=v->instance;
    else if(!std::strcmp(key,kOfxImagePropData)) *p=v->data;
    else return kOfxStatErrUnknown;
    return kOfxStatOK;
}
OfxStatus getInt(OfxPropertySetHandle h,const char* key,int,int* p) {
    if(std::strcmp(key,kOfxImagePropRowBytes)) return kOfxStatErrUnknown;
    *p=static_cast<Props*>(h)->stride; return kOfxStatOK;
}
OfxStatus getInts(OfxPropertySetHandle h,const char* key,int n,int* p) {
    auto* v=static_cast<Props*>(h);
    if(n!=4) return kOfxStatErrValue;
    if(!std::strcmp(key,kOfxImagePropBounds)) std::copy(v->bounds,v->bounds+4,p);
    else if(!std::strcmp(key,kOfxImageEffectPropRenderWindow)) std::copy(v->window,v->window+4,p);
    else return kOfxStatErrUnknown;
    return kOfxStatOK;
}
OfxStatus getStr(OfxPropertySetHandle h,const char* key,int,char** p) {
    auto* v=static_cast<Props*>(h);
    if(!std::strcmp(key,kOfxImageEffectPropPixelDepth)) *p=v->depth.data();
    else if(!std::strcmp(key,kOfxImageEffectPropComponents)) *p=v->components.data();
    else if(!std::strcmp(key,kOfxPropName)) *p=v->name.data();
    else return kOfxStatErrUnknown;
    return kOfxStatOK;
}
OfxStatus getDouble(OfxPropertySetHandle h,const char*,int,double* p) { *p=static_cast<Props*>(h)->time; return kOfxStatOK; }
OfxStatus getProps(OfxImageEffectHandle h,OfxPropertySetHandle* p) { *p=h;return kOfxStatOK; }
OfxStatus getParams(OfxImageEffectHandle,OfxParamSetHandle* p) { *p=nullptr;return kOfxStatOK; }
OfxStatus getClip(OfxImageEffectHandle,const char* name,OfxImageClipHandle* p,OfxPropertySetHandle*) {
    *p=!std::strcmp(name,"Source") ? &sourceImage : &destImage; return kOfxStatOK;
}
OfxStatus getImage(OfxImageClipHandle h,OfxTime,const OfxRectD*,OfxPropertySetHandle* p) { *p=h;return kOfxStatOK; }
OfxStatus releaseImage(OfxPropertySetHandle) { ++released;return kOfxStatOK; }
OfxStatus abortRender(OfxImageEffectHandle) { return 0; }
OfxPropertySuiteV1 properties{};
OfxImageEffectSuiteV1 images{};
OfxParameterSuiteV1 parameters{};
void* fetch(OfxPropertySetHandle,const char* name,int) {
    if(!std::strcmp(name,kOfxPropertySuite)) return &properties;
    if(!std::strcmp(name,kOfxImageEffectSuite)) return &images;
    if(!std::strcmp(name,kOfxParameterSuite)) return &parameters;
    return nullptr;
}
int main() {
    properties.propSetPointer=setPtr; properties.propGetPointer=getPtr;
    properties.propGetInt=getInt; properties.propGetIntN=getInts;
    properties.propGetString=getStr; properties.propGetDouble=getDouble;
    images.getPropertySet=getProps; images.getParamSet=getParams;
    images.clipGetHandle=getClip; images.clipGetImage=getImage;
    images.clipReleaseImage=releaseImage; images.abort=abortRender;
    OfxHost host{nullptr,fetch};
    auto* plugin=OfxGetPlugin(0); plugin->setHost(&host);
    if(plugin->mainEntry(kOfxActionLoad,nullptr,nullptr,nullptr)!=kOfxStatOK) return 1;
    if(plugin->mainEntry(kOfxActionCreateInstance,&effect,nullptr,nullptr)!=kOfxStatOK || !effect.instance) return 2;
    std::vector<float> src(4*36),dst(4*40,-77);
    for(size_t i=0;i<src.size();++i) src[i]=float(i)/100;
    sourceImage.data=src.data()+3*36; sourceImage.stride=-36*sizeof(float);
    destImage.data=dst.data()+3*40; destImage.stride=-40*sizeof(float);
    args.window[0]=12;args.window[2]=16; // Subwindow, nonzero image origin and negative strides.
    if(plugin->mainEntry(kOfxImageEffectActionRender,&effect,&args,nullptr)!=kOfxStatOK) return 3;
    for(int y=0;y<4;++y) for(int x=0;x<40;++x) {
        const float expected=(x>=8 && x<24) ? src[y*36+x] : -77;
        if(dst[y*40+x]!=expected) return 4;
    }
    if(released!=2) return 5;
    sourceImage.depth=kOfxBitDepthHalf;
    if(plugin->mainEntry(kOfxImageEffectActionRender,&effect,&args,nullptr)!=kOfxStatErrFormat || released!=4) return 6;
    sourceImage.depth=kOfxBitDepthFloat;
    args.name=AIDepthPro::Params::kParamClearCache;
    if(plugin->mainEntry(kOfxActionInstanceChanged,&effect,&args,nullptr)!=kOfxStatOK) return 7;
    if(plugin->mainEntry(kOfxActionPurge,&effect,nullptr,nullptr)!=kOfxStatOK) return 8;
    if(plugin->mainEntry(kOfxActionDestroyInstance,&effect,nullptr,nullptr)!=kOfxStatOK || effect.instance) return 9;
    std::cout<<"OFX lifecycle, partial window, signed/padded strides, identity bypass and image release passed\n";
    return 0;
}
