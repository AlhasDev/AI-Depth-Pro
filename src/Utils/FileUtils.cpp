#include "FileUtils.h"
#include "Logger.h"
#include <fstream>
#include <filesystem>
#include <cstdlib>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <dlfcn.h>
#else
#include <dlfcn.h>
#endif

namespace fs = std::filesystem;

namespace AIDepthPro {

bool FileUtils::fileExists(const std::string& path) {
    if (path.empty()) return false;
    std::error_code ec;
    return fs::exists(path, ec) && fs::is_regular_file(path, ec);
}

size_t FileUtils::getFileSize(const std::string& path) {
    if (!fileExists(path)) return 0;
    std::error_code ec;
    return static_cast<size_t>(fs::file_size(path, ec));
}

std::string FileUtils::getDirectory(const std::string& filePath) {
    fs::path p(filePath);
    return p.parent_path().string();
}

std::string FileUtils::joinPath(const std::string& dir, const std::string& filename) {
    fs::path p(dir);
    p /= filename;
    return p.string();
}

std::string FileUtils::getPluginDirectory() {
#if defined(_WIN32)
    HMODULE hm = NULL;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)&FileUtils::getPluginDirectory, &hm)) {
        char path[MAX_PATH];
        if (GetModuleFileNameA(hm, path, sizeof(path)) > 0) {
            return getDirectory(path);
        }
    }
    return "";
#else
    Dl_info info{};
    if (dladdr(reinterpret_cast<void*>(&FileUtils::getPluginDirectory), &info) && info.dli_fname)
        return getDirectory(info.dli_fname);
    return "";
#endif
}

std::vector<std::string> FileUtils::getModelSearchPaths() {
    std::vector<std::string> paths;

    // 1. Environment variable override
    const char* envPath = std::getenv("AI_DEPTH_PRO_MODELS_DIR");
    if (envPath && std::string(envPath).length() > 0) {
        paths.push_back(envPath);
    }

    // 2. Relative to plugin bundle binary directory
    std::string pluginDir = getPluginDirectory();
    if (!pluginDir.empty()) {
        paths.push_back(pluginDir);
        paths.push_back(joinPath(pluginDir, "models"));
        paths.push_back(joinPath(pluginDir, "../models"));
        paths.push_back(joinPath(pluginDir, "../../models"));
        paths.push_back(joinPath(pluginDir, "../../../models"));
        paths.push_back(joinPath(pluginDir, "../Resources/models"));
    }

    // 3. Common Resolve / User Application Data paths
#if defined(_WIN32)
    const char* appData = std::getenv("APPDATA");
    if (appData) {
        paths.push_back(joinPath(appData, "AI-Depth-Pro/models"));
        paths.push_back(joinPath(appData, "Blackmagic Design/DaVinci Resolve/Support/AI-Depth-Pro/models"));
    }
    const char* progData = std::getenv("PROGRAMDATA");
    if (progData) {
        paths.push_back(joinPath(progData, "Blackmagic Design/DaVinci Resolve/Support/AI-Depth-Pro/models"));
        paths.push_back(joinPath(progData, "OFX/Plugins/AI-Depth-Pro/models"));
    }
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    if (home) {
        paths.push_back(joinPath(home, "Library/Application Support/Blackmagic Design/DaVinci Resolve/AI-Depth-Pro/models"));
        paths.push_back(joinPath(home, "Library/OFX/Plugins/AI-Depth-Pro.ofx.bundle/Contents/Resources/models"));
    }
    paths.push_back("/Library/Application Support/Blackmagic Design/DaVinci Resolve/AI-Depth-Pro/models");
    paths.push_back("/Library/OFX/Plugins/AI-Depth-Pro.ofx.bundle/Contents/Resources/models");
#endif

    // 4. Current working directory & scratch models
    paths.push_back("models");
    paths.push_back("./models");
    paths.push_back("../models");

    return paths;
}

std::string FileUtils::findModelFile(const std::string& modelNameOrPath) {
    if (modelNameOrPath.empty()) return "";

    // If given an absolute or valid relative direct path
    if (fileExists(modelNameOrPath)) {
        return modelNameOrPath;
    }

    // Search across candidate directories
    std::vector<std::string> searchDirs = getModelSearchPaths();
    std::vector<std::string> extensions = {"", ".onnx", ".engine", ".mlmodel"};

    for (const auto& dir : searchDirs) {
        for (const auto& ext : extensions) {
            std::string candidate = joinPath(dir, modelNameOrPath + ext);
            if (fileExists(candidate)) {
                LOG_INFO("Model found: " + candidate);
                return candidate;
            }
        }
    }

    return "";
}

std::vector<std::string> FileUtils::discoverAvailableModels() {
    std::vector<std::string> models;
    std::vector<std::string> searchDirs = getModelSearchPaths();

    for (const auto& dir : searchDirs) {
        std::error_code ec;
        if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) continue;

        for (const auto& entry : fs::directory_iterator(dir, ec)) {
            if (entry.is_regular_file(ec)) {
                std::string ext = entry.path().extension().string();
                if (ext == ".onnx" || ext == ".engine" || ext == ".mlmodel") {
                    models.push_back(entry.path().string());
                }
            }
        }
    }
    return models;
}

} // namespace AIDepthPro
