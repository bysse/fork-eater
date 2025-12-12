#pragma once

#include <string>
#include <vector>
#include <set>
#include <functional>

class ShaderPreprocessor {
public:
    struct LineMapping {
        int preprocessedLine = 0;   // 1-based line number in the flattened shader source
        std::string filePath;       // Original file path
        int fileLine = 0;           // 1-based line number in the original file
    };

    struct PreprocessResult {
        std::string source;
        std::vector<std::string> includedFiles;
        std::vector<std::string> switchFlags;
        std::vector<LineMapping> lineMappings;
    };

    // Callback for logging errors/warnings during preprocessing
    std::function<void(const std::string&)> onMessage;

    ShaderPreprocessor();

    // Preprocesses a shader file, resolving #pragma include directives.
    PreprocessResult preprocess(const std::string& filePath);

private:
    // Recursive helper for preprocessing
    std::string preprocessRecursive(const std::string& filePath,
                                    std::vector<std::string>& includeStack,
                                    std::set<std::string>& uniqueIncludedFiles,
                                    std::vector<std::string>& switchFlags,
                                    std::vector<LineMapping>& lineMappings,
                                    int& currentLine);
};
