#include "Cache/PersistentDepthCache.h"
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace AIDepthPro;
int main() {
    const auto dir = std::filesystem::temp_directory_path() / "ai_depth_pro_cache_test";
#ifdef _WIN32
    _putenv_s("AI_DEPTH_PRO_CACHE_DIR", dir.string().c_str());
#else
    setenv("AI_DEPTH_PRO_CACHE_DIR", dir.string().c_str(), 1);
#endif
    PersistentDepthCache::clear();
    CacheKey key; key.time=12.0; key.width=3; key.height=2;
    key.modelPath="model-v1"; key.sourceId="frame-A";
    DepthFrame source(3,2); source.data={0.0f,0.2f,0.4f,0.6f,0.8f,1.0f};
    if (!PersistentDepthCache::store(key,source)) return 1;
    DepthFrame loaded;
    if (!PersistentDepthCache::load(key,loaded) || loaded.data.size()!=source.data.size()) return 2;
    for (size_t i=0;i<source.data.size();++i)
        if (std::abs(source.data[i]-loaded.data[i])>0.00002f) return 3;
    key.sourceId="frame-B";
    if (PersistentDepthCache::load(key,loaded)) return 4;
    key.sourceId="frame-A"; key.modelPath="model-v2";
    if (PersistentDepthCache::load(key,loaded)) return 5;
    PersistentDepthCache::clear();
    key.modelPath="model-v1";
    if (PersistentDepthCache::load(key,loaded)) return 6;
    std::cout << "Persistent cache roundtrip and invalidation passed\n";
    return 0;
}
