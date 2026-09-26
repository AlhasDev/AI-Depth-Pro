#include "LRUCache.h"
#include <algorithm>

namespace AIDepthPro {

LRUCache::LRUCache(size_t maxFrames, size_t maxMemoryMB)
    : m_maxFrames(maxFrames), m_maxMemoryBytes(maxMemoryMB * 1024 * 1024) {}

LRUCache::~LRUCache() {
    clear();
}

bool LRUCache::get(const CacheKey& key, DepthFrame& outFrame) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_map.find(key);
    if (it == m_map.end()) {
        m_misses++;
        return false;
    }

    // Move to front of LRU list
    m_items.splice(m_items.begin(), m_items, it->second);
    outFrame = it->second->frame;
    m_hits++;
    return true;
}

void LRUCache::put(const CacheKey& key, const DepthFrame& frame) {
    std::lock_guard<std::mutex> lock(m_mutex);

    size_t frameSizeBytes = sizeof(DepthFrame) + frame.data.size() * sizeof(float);

    auto it = m_map.find(key);
    if (it != m_map.end()) {
        m_currentMemoryBytes -= it->second->sizeBytes;
        m_items.erase(it->second);
        m_map.erase(it);
    }
    if (!frame.isValid() || m_maxFrames == 0 || frameSizeBytes > m_maxMemoryBytes) return;
    // Check capacity and evict least recently used elements
    while (!m_items.empty() && (m_items.size() >= m_maxFrames || (m_currentMemoryBytes + frameSizeBytes > m_maxMemoryBytes))) {
        auto last = m_items.end();
        --last;
        m_currentMemoryBytes -= last->sizeBytes;
        m_map.erase(last->key);
        m_items.pop_back();
    }

    // Insert new entry at the front
    CacheEntry entry;
    entry.key = key;
    entry.frame = frame;
    entry.sizeBytes = frameSizeBytes;

    m_items.push_front(std::move(entry));
    m_map[key] = m_items.begin();
    m_currentMemoryBytes += frameSizeBytes;
}

void LRUCache::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_items.clear();
    m_map.clear();
    m_currentMemoryBytes = 0;
}

void LRUCache::setCapacity(size_t maxFrames, size_t maxMemoryMB) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxFrames = maxFrames;
    m_maxMemoryBytes = maxMemoryMB * 1024 * 1024;
    while (!m_items.empty() && (m_items.size() > m_maxFrames || m_currentMemoryBytes > m_maxMemoryBytes)) {
        auto last = std::prev(m_items.end());
        m_currentMemoryBytes -= last->sizeBytes;
        m_map.erase(last->key);
        m_items.pop_back();
    }
}

size_t LRUCache::getFrameCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_items.size();
}

size_t LRUCache::getMemoryUsageBytes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentMemoryBytes;
}

float LRUCache::getHitRate() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t total = m_hits + m_misses;
    if (total == 0) return 0.0f;
    return static_cast<float>(m_hits) / static_cast<float>(total);
}

} // namespace AIDepthPro
