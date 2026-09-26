#include "../src/DepthEngine/OnnxDepthEngine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
using namespace AIDepthPro;
int main() {
    OnnxDepthEngine engine(EngineType::CPU);
    EngineConfig config;
    config.modelPath=TEST_MODEL_PATH; config.qualityMode=QualityMode::Fast;
    if (!engine.Initialize(config)) return 1;
    const int w=320,h=240;
    std::vector<float> pixels(w*h*4,1.0f);
    ImageFrame image{pixels.data(),w,h,w*4*sizeof(float),BitDepth::Float,PixelComponent::RGBA};
    for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
        auto i=(y*w+x)*4;
        pixels[i]=float(x)/w; pixels[i+1]=float(y)/h;
        pixels[i+2]=(x>80 && x<160 && y>50 && y<180) ? 1.0f : 0.15f;
    }
    DepthFrame a,b;
    if(!engine.GenerateDepth(image,a) || !a.isValid()) return 2;
    const float first=engine.GetStats().totalFrameTimeMs;
    for(int y=0;y<h;++y) for(int x=0;x<w;++x)
        for(int c=0;c<3;++c) pixels[(y*w+x)*4+c]=((x/24+y/24)%2) ? 0.9f : 0.1f;
    if(!engine.GenerateDepth(image,b) || !b.isValid()) return 3;
    double difference=0;
    for(size_t i=0;i<a.data.size();++i) {
        if(!std::isfinite(a.data[i]) || !std::isfinite(b.data[i]) || a.data[i]<0 || a.data[i]>1) return 4;
        difference+=std::abs(a.data[i]-b.data[i]);
    }
    difference/=a.data.size();
    std::cout<<"Native CPU inference: first="<<first<<" ms, second="<<engine.GetStats().totalFrameTimeMs
             <<" ms, mean image-dependent depth difference="<<difference<<"\n";
    if(difference<0.001) return 5;
    for (int i=0;i<2;++i) {
        const auto& d=i==0 ? a : b;
        std::ofstream file(i==0 ? "depth-a.pgm" : "depth-b.pgm",std::ios::binary);
        file<<"P5\n"<<w<<" "<<h<<"\n255\n";
        for(float value:d.data) { unsigned char byte=static_cast<unsigned char>(value*255); file.write(reinterpret_cast<char*>(&byte),1); }
    }
    engine.Shutdown();
    if(engine.IsReady() || engine.GenerateDepth(image,a) || a.isValid()) return 6;
    return 0;
}
