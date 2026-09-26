#include "../src/Cache/LRUCache.h"
#include "../src/Temporal/TemporalStabilizer.h"
#include <algorithm>
#include <cmath>
#include <string>
extern void reportTest(const std::string&, bool, const std::string& = "");
void runRegressionTests() {
    using namespace AIDepthPro;
    CacheKey key;
    DepthFrame frame(64,64), retrieved;
    LRUCache zero(0,1); zero.put(key,frame);
    reportTest("Zero-capacity cache stays empty", zero.getFrameCount()==0);
    LRUCache limited(2,1); limited.put(key,DepthFrame(1024,1024));
    reportTest("Oversized frame does not exceed memory budget", limited.getMemoryUsageBytes()==0);
    limited.put(key,frame); limited.setCapacity(0,0);
    reportTest("Shrinking cache immediately evicts frames", limited.getFrameCount()==0);
    limited.setCapacity(2,1); limited.put(key,frame); limited.put(key,DepthFrame(1024,1024));
    reportTest("Replacing an entry respects memory budget", limited.getMemoryUsageBytes()<=1024*1024);
    limited.put(key,frame); CacheKey changed=key; changed.sourceId="changed-source";
    reportTest("Upstream image changes invalidate inference cache", !limited.get(changed,retrieved));
    std::vector<float> image(16*16*4,0.5f);
    ImageFrame img{image.data(),16,16,16*4*sizeof(float),BitDepth::Float,PixelComponent::RGBA};
    DepthFrame depth(16,16), stable;
    std::fill(depth.data.begin(),depth.data.end(),0.2f);
    TemporalStabilizer stabilizer;
    stabilizer.Stabilize(img,depth,stable,0.8f,false,1.0f);
    stabilizer.Stabilize(img,depth,stable,0.8f,false,1.0f);
    reportTest("Identical first two frames keep depth brightness", std::abs(stable.data[0]-0.2f)<1e-6f);
}
