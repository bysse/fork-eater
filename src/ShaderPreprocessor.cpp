#include "ShaderPreprocessor.h"
#include "Logger.h" // For LOG_ERROR
#include "GeneratedShaderLibraries.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <algorithm>

// Helper function to read a file's content
static std::string readFileContent(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

ShaderPreprocessor::ShaderPreprocessor() {
    // Default empty onMessage callback
    onMessage = [](const std::string& msg) { LOG_ERROR("ShaderPreprocessor: {}", msg); };

    // Initialize embedded libraries
    EmbeddedLibraries::initialize();
}

std::string ShaderPreprocessor::preprocess(const std::string& filePath, std::vector<std::string>& includedFiles) {
    includedFiles.clear();
    std::vector<std::string> includeStack; // To detect circular dependencies
    std::set<std::string> uniqueIncludedFiles; // To collect all unique included files

    std::string preprocessedSource = preprocessRecursive(filePath, includeStack, uniqueIncludedFiles);

    LOG_DEBUG("Preprocessed source for {}:\n{}", filePath, preprocessedSource);

    // Convert set to vector
    for (const auto& file : uniqueIncludedFiles) {
        includedFiles.push_back(file);
    }
    
    return preprocessedSource;
}

std::string ShaderPreprocessor::preprocessRecursive(const std::string& filePath,
                                                    std::vector<std::string>& includeStack,
                                                    std::set<std::string>& uniqueIncludedFiles) {
    LOG_DEBUG("Preprocessing file: {}", filePath);
    // Check for include loops
    if (std::find(includeStack.begin(), includeStack.end(), filePath) != includeStack.end()) {
        std::string errorMsg = "Include loop detected: " + filePath;
        if (onMessage) onMessage(errorMsg);
        return "#error " + errorMsg + "\n";
    }

    includeStack.push_back(filePath);
    uniqueIncludedFiles.insert(filePath);

    std::string source = readFileContent(filePath);
    if (source.empty()) {
        // Try to load from embedded libraries
        auto it = EmbeddedLibraries::g_libs.find(std::filesystem::path(filePath).filename().string());
        if (it != EmbeddedLibraries::g_libs.end()) {
            source = std::string(it->second.first, it->second.second);
        } else {
            includeStack.pop_back();
            std::string errorMsg = "Failed to read included file: " + filePath;
            if (onMessage) onMessage(errorMsg);
            return "#error " + errorMsg + "\n";
        }
    }

    LOG_DEBUG("Source for {}:\n{}", filePath, source);

    std::stringstream preprocessedSource;
    std::stringstream ss(source);
    std::string line;
    std::regex includeRegex("#pragma include\(\"<([a-zA-Z0-9_./-]+)>\")");
    
    while (std::getline(ss, line)) {
        std::smatch matches;
        if (std::regex_search(line, matches, includeRegex)) {
            if (matches.size() == 2) {
                std::string includeFileName = matches[1].str();
                std::filesystem::path currentDirPath = std::filesystem::path(filePath).parent_path(); // Use parent_path
                std::string includePath = (currentDirPath / includeFileName).string();

                // Recursively preprocess included file
                std::string includedContent = preprocessRecursive(includePath, includeStack, uniqueIncludedFiles);
                preprocessedSource << includedContent;
            } else {
                std::string errorMsg = "Invalid include directive: " + line;
                if (onMessage) onMessage(errorMsg);
                preprocessedSource << "#error " + errorMsg + "\n";
            }
        } else {
            preprocessedSource << line << "\n";
        }
    }

    includeStack.pop_back();
    return preprocessedSource.str();
}
