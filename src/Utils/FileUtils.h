#pragma once

#include <string>
#include <vector>

namespace AIDepthPro {

class FileUtils {
public:
    static bool fileExists(const std::string& path);
    static size_t getFileSize(const std::string& path);
    static std::string getDirectory(const std::string& filePath);
    static std::string joinPath(const std::string& dir, const std::string& filename);
    static std::string getPluginDirectory();
    static std::vector<std::string> getModelSearchPaths();
    static std::string findModelFile(const std::string& modelNameOrPath);
    static std::vector<std::string> discoverAvailableModels();
};

} // namespace AIDepthPro
