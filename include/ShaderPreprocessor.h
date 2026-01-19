#pragma once

#include <string>
#include <vector>
#include <set>
#include <functional>
#include "RenderScaleMode.h"

class ShaderPreprocessor {
public:
    struct LineMapping {
        int preprocessedLine = 0;   // 1-based line number in the flattened shader source
        std::string filePath;       // Original file path
        int fileLine = 0;           // 1-based line number in the original file
    };

    struct SwitchInfo {
        std::string name;
        bool defaultValue = false;
    };

    struct UniformRange {
        std::string name;
        float min = 0.0f;
        float max = 1.0f;
    };

    struct LabelInfo {
        std::string name;
        std::string label;
    };

    struct PreprocessResult {
        std::string source;
        std::vector<std::string> includedFiles;
        std::vector<SwitchInfo> switchFlags;
        std::vector<UniformRange> uniformRanges;
        std::vector<LabelInfo> labels;
        std::vector<LineMapping> lineMappings;
    };

    // Callback for logging errors/warnings during preprocessing
    std::function<void(const std::string&)> onMessage;

    ShaderPreprocessor();

    // Preprocesses a shader file, resolving #pragma include directives.
    PreprocessResult preprocess(const std::string& filePath, RenderScaleMode scaleMode = RenderScaleMode::Resolution);

private:
    // Recursive helper for preprocessing
    std::string preprocessRecursive(const std::string& filePath,
                                    std::vector<std::string>& includeStack,
                                    std::set<std::string>& uniqueIncludedFiles,
                                    std::vector<SwitchInfo>& switchFlags,
                                    std::vector<UniformRange>& uniformRanges,
                                    std::vector<LabelInfo>& labels,
                                    std::vector<LineMapping>& lineMappings,
                                    int& currentLine);

    // Helper for preprocessing source content (used by preprocessRecursive and for embedded libs)
    std::string preprocessSource(const std::string& source,
                                 const std::string& filePath,
                                 std::vector<std::string>& includeStack,
                                 std::set<std::string>& uniqueIncludedFiles,
                                 std::vector<SwitchInfo>& switchFlags,
                                 std::vector<UniformRange>& uniformRanges,
                                 std::vector<LabelInfo>& labels,
                                 std::vector<LineMapping>& lineMappings,
                                 int& currentLine);
};
