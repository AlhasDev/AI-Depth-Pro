#pragma once
#include "LRUCache.h"

namespace AIDepthPro {
class PersistentDepthCache {
public:
    static bool load(const CacheKey& key, DepthFrame& frame);
    static bool store(const CacheKey& key, const DepthFrame& frame);
    static void clear();
};
}
