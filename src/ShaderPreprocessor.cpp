#include "ShaderPreprocessor.h"
#include "Logger.h" // For LOG_ERROR
#include "GeneratedShaderLibraries.h"
#include "ShaderManager.h"
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

ShaderPreprocessor::PreprocessResult ShaderPreprocessor::preprocess(const std::string& filePath) {
    PreprocessResult result;
    std::vector<std::string> includeStack;
    std::set<std::string> uniqueIncludedFiles;

    result.source = preprocessRecursive(filePath, includeStack, uniqueIncludedFiles, result.switchFlags);

    for (const auto& file : uniqueIncludedFiles) {
        result.includedFiles.push_back(file);
    }
    
    return result;
}

std::string ShaderPreprocessor::preprocessRecursive(const std::string& filePath,
                                                    std::vector<std::string>& includeStack,
                                                    std::set<std::string>& uniqueIncludedFiles,
                                                    std::vector<std::string>& switchFlags) {
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
        includeStack.pop_back();
        std::string errorMsg = "Failed to read file: " + filePath;
        if (onMessage) onMessage(errorMsg);
        return "#error " + errorMsg + "\n";
    }

    LOG_DEBUG("Source for {}:\n{}", filePath, source);

    std::stringstream preprocessedSource;
    std::stringstream ss(source);
    std::string line;
    std::regex includeRegex(R"x(#pragma\s+include\s*\(([^)]+)\))x");
    std::regex switchRegex(R"x(#pragma\s+switch\s*\(([^)]+)\))x");
    
    while (std::getline(ss, line)) {
        std::smatch matches;
        if (std::regex_search(line, matches, includeRegex)) {
            if (matches.size() == 2) {
                std::string includeFileName = matches[1].str();
                std::string includedContent;

                if (includeFileName.rfind("lib/", 0) == 0) {
                    std::filesystem::path projectRoot = std::filesystem::path(filePath).parent_path().parent_path();
                    std::filesystem::path libDir = projectRoot / "lib";
                    std::filesystem::create_directories(libDir);
                    std::string libFileName = includeFileName.substr(4);
                    std::filesystem::path libFilePath = libDir / libFileName;

                    if (!std::filesystem::exists(libFilePath)) {
                        auto it = EmbeddedLibraries::g_libs.find(libFileName);
                        if (it != EmbeddedLibraries::g_libs.end()) {
                            std::ofstream outFile(libFilePath);
                            outFile.write(it->second.first, it->second.second);
                            outFile.close();
                        }
                    }
                    includedContent = preprocessRecursive(libFilePath.string(), includeStack, uniqueIncludedFiles, switchFlags);
                } else {
                    // If not in embedded libs, try to read from filesystem relative to current file
                    std::filesystem::path currentDirPath = std::filesystem::path(filePath).parent_path();
                    std::string includePath = (currentDirPath / includeFileName).string();
                    includedContent = preprocessRecursive(includePath, includeStack, uniqueIncludedFiles, switchFlags);
                }
                preprocessedSource << includedContent;
            } else {
                std::string errorMsg = "Invalid include directive: " + line;
                if (onMessage) onMessage(errorMsg);
                preprocessedSource << "#error " + errorMsg + "\n";
            }
        } else if (std::regex_search(line, matches, switchRegex)) {
            if (matches.size() == 2) {
                std::string switchName = matches[1].str();
                switchFlags.push_back(switchName);
            } else {
                std::string errorMsg = "Invalid switch directive: " + line;
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
