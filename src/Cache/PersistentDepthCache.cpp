#include "PersistentDepthCache.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace AIDepthPro {
namespace {
namespace fs = std::filesystem;
constexpr uint64_t basis = 14695981039346656037ull;
uint64_t add(uint64_t h, const void* data, size_t size) {
    const auto* p = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < size; ++i) { h ^= p[i]; h *= 1099511628211ull; }
    return h;
}
template<class T> uint64_t addValue(uint64_t h, const T& value) { return add(h, &value, sizeof(value)); }
uint64_t keyHash(const CacheKey& key) {
    uint64_t h = basis;
    h = addValue(h, key.time); h = addValue(h, key.width); h = addValue(h, key.height);
    const int quality = static_cast<int>(key.quality);
    h = addValue(h, quality); h = addValue(h, key.settingsHash);
    h = add(h, key.modelPath.data(), key.modelPath.size());
    h = add(h, key.sourceId.data(), key.sourceId.size());
    return h;
}
fs::path directory() {
    if (const char* overridePath = std::getenv("AI_DEPTH_PRO_CACHE_DIR"))
        if (*overridePath) return fs::path(overridePath);
#ifdef _WIN32
    if (const char* local = std::getenv("LOCALAPPDATA"))
        if (*local) return fs::path(local) / "AI-Depth-Pro" / "DepthCache";
#else
    if (const char* xdg = std::getenv("XDG_CACHE_HOME"))
        if (*xdg) return fs::path(xdg) / "AI-Depth-Pro" / "DepthCache";
    if (const char* home = std::getenv("HOME"))
        if (*home) return fs::path(home) / ".cache" / "AI-Depth-Pro" / "DepthCache";
#endif
    return fs::temp_directory_path() / "AI-Depth-Pro" / "DepthCache";
}
fs::path pathFor(const CacheKey& key) {
    std::ostringstream name;
    name << std::hex << std::setw(16) << std::setfill('0') << keyHash(key) << ".depth";
    return directory() / name.str();
}
struct Header { char magic[8]; uint32_t width; uint32_t height; uint64_t key; uint64_t checksum; };
}
bool PersistentDepthCache::load(const CacheKey& key, DepthFrame& frame) {
    try {
        const fs::path path = pathFor(key);
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;
        Header header{};
        in.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!in || std::memcmp(header.magic, "ADPCv001", 8) ||
            header.width != static_cast<uint32_t>(key.width) ||
            header.height != static_cast<uint32_t>(key.height) || header.key != keyHash(key)) return false;
        const size_t count = size_t(key.width) * size_t(key.height);
        if (key.width <= 0 || key.height <= 0 || count > (size_t(1) << 29) ||
            fs::file_size(path) != sizeof(header) + count * sizeof(uint16_t)) return false;
        std::vector<uint16_t> pixels(count);
        in.read(reinterpret_cast<char*>(pixels.data()), std::streamsize(count * sizeof(uint16_t)));
        if (!in || add(basis, pixels.data(), count * sizeof(uint16_t)) != header.checksum) return false;
        frame.width = key.width; frame.height = key.height; frame.data.resize(count);
        for (size_t i = 0; i < count; ++i) frame.data[i] = float(pixels[i]) / 65535.0f;
        return true;
    } catch (...) { return false; }
}
bool PersistentDepthCache::store(const CacheKey& key, const DepthFrame& frame) {
    if (!frame.isValid() || frame.width != key.width || frame.height != key.height) return false;
    try {
        fs::create_directories(directory());
        const fs::path destination = pathFor(key);
        static std::atomic<uint64_t> sequence{0};
        fs::path temporary = destination;
        temporary += ".tmp." + std::to_string(++sequence);
        std::vector<uint16_t> pixels(frame.data.size());
        for (size_t i = 0; i < pixels.size(); ++i) {
            const float v = std::isfinite(frame.data[i]) ? frame.data[i] : 0.0f;
            pixels[i] = static_cast<uint16_t>(std::lround(std::clamp(v, 0.0f, 1.0f) * 65535.0f));
        }
        Header header{};
        std::memcpy(header.magic, "ADPCv001", 8);
        header.width = static_cast<uint32_t>(frame.width); header.height = static_cast<uint32_t>(frame.height);
        header.key = keyHash(key);
        header.checksum = add(basis, pixels.data(), pixels.size() * sizeof(uint16_t));
        {
            std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
            if (!out) return false;
            out.write(reinterpret_cast<const char*>(&header), sizeof(header));
            out.write(reinterpret_cast<const char*>(pixels.data()), std::streamsize(pixels.size() * sizeof(uint16_t)));
            if (!out) { out.close(); fs::remove(temporary); return false; }
        }
        std::error_code ec;
        fs::rename(temporary, destination, ec);
        if (ec) { fs::remove(destination, ec); ec.clear(); fs::rename(temporary, destination, ec); }
        if (ec) fs::remove(temporary);
        return !ec;
    } catch (...) { return false; }
}
void PersistentDepthCache::clear() {
    try { fs::remove_all(directory()); } catch (...) {}
}
}
