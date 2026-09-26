#include "../src/Processing/EdgeRefinement.h"
#include "../src/Temporal/MotionEstimator.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
using namespace AIDepthPro;
template<class F> double benchmark(F f) {
    f();
    std::vector<double> times;
    for(int i=0;i<5;++i) {
        auto start=std::chrono::steady_clock::now(); f();
        times.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
    }
    std::sort(times.begin(),times.end()); return times[times.size()/2];
}
int main(int argc,char** argv) {
    const int w=1920,h=1080;
    std::vector<float> pixels(w*h*4,1),other(w*h*4,1);
    DepthFrame raw(w,h),refined;
    for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
        float value=float((x*13+y*7)%255)/255;
        for(int c=0;c<3;++c) pixels[(y*w+x)*4+c]=value;
        raw.set(x,y,0.5f+0.4f*std::sin(x*0.013f)*std::cos(y*0.011f));
    }
    other=pixels;
    ImageFrame image{pixels.data(),w,h,w*4*sizeof(float),BitDepth::Float,PixelComponent::RGBA};
    ImageFrame prev=image; prev.data=other.data();
    MotionField motion;
    auto filterMs=benchmark([&]{EdgeRefinement::ApplyGuidedFilter(image,raw,refined,4,0.001f);});
    auto motionMs=benchmark([&]{MotionEstimator::EstimateMotion(prev,image,motion,8);});
    std::cout<<"1920x1080 float RGBA, 1 warmup + 5 measured iterations, median\n"
             <<"guided_filter_ms="<<filterMs<<"\nmotion_estimator_ms="<<motionMs<<"\n";
    if(argc>1 && std::string(argv[1])=="--save") {
        std::ofstream out("guided-reference.bin",std::ios::binary);
        out.write(reinterpret_cast<char*>(refined.data.data()),refined.data.size()*sizeof(float));
    } else if(argc>1 && std::string(argv[1])=="--compare") {
        std::vector<float> reference(refined.data.size());
        std::ifstream in("guided-reference.bin",std::ios::binary);
        if(!in.read(reinterpret_cast<char*>(reference.data()),reference.size()*sizeof(float))) return 1;
        double maxError=0,meanError=0;
        for(size_t i=0;i<reference.size();++i) {
            double diff=std::abs(reference[i]-refined.data[i]);
            maxError=std::max(maxError,diff); meanError+=diff;
        }
        std::cout<<"guided_filter_max_difference="<<maxError<<"\nguided_filter_mean_difference="<<meanError/reference.size()<<"\n";
        if(maxError>0.001) return 2;
    }
    return 0;
}
