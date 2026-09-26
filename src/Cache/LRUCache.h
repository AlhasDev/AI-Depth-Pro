#pragma once

#include "../Core/Types.h"
#include <unordered_map>
#include <list>
#include <mutex>
#include <string>

namespace AIDepthPro {

struct CacheKey {
    double time = 0.0;
    int width = 0;
    int height = 0;
    QualityMode quality = QualityMode::Balanced;
    uint64_t settingsHash = 0;
    std::string modelPath;
    std::string sourceId;

    bool operator==(const CacheKey& o) const {
        return time == o.time &&
               width == o.width &&
               height == o.height &&
               quality == o.quality &&
               settingsHash == o.settingsHash &&
               modelPath == o.modelPath && sourceId == o.sourceId;
    }
};

struct CacheKeyHash {
    std::size_t operator()(const CacheKey& k) const {
        std::size_t h1 = std::hash<double>{}(k.time);
        std::size_t h2 = std::hash<int>{}(k.width ^ (k.height << 16));
        std::size_t h3 = std::hash<int>{}(static_cast<int>(k.quality));
        std::size_t h4 = std::hash<uint64_t>{}(k.settingsHash);
        std::size_t h5 = std::hash<std::string>{}(k.modelPath);
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ std::hash<std::string>{}(k.sourceId);
    }
};

class LRUCache {
public:
    explicit LRUCache(size_t maxFrames = 128, size_t maxMemoryMB = 1024);
    ~LRUCache();

    bool get(const CacheKey& key, DepthFrame& outFrame);
    void put(const CacheKey& key, const DepthFrame& frame);
    void clear();

    void setCapacity(size_t maxFrames, size_t maxMemoryMB);
    size_t getFrameCount() const;
    size_t getMemoryUsageBytes() const;
    size_t getHits() const { std::lock_guard<std::mutex> lock(m_mutex); return m_hits; }
    size_t getMisses() const { std::lock_guard<std::mutex> lock(m_mutex); return m_misses; }
    float getHitRate() const;

private:
    struct CacheEntry {
        CacheKey key;
        DepthFrame frame;
        size_t sizeBytes = 0;
    };

    size_t m_maxFrames;
    size_t m_maxMemoryBytes;
    size_t m_currentMemoryBytes = 0;

    std::list<CacheEntry> m_items;
    std::unordered_map<CacheKey, std::list<CacheEntry>::iterator, CacheKeyHash> m_map;

    mutable std::mutex m_mutex;
    size_t m_hits = 0;
    size_t m_misses = 0;
};

} // namespace AIDepthPro
