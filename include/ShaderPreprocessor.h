#pragma once

#include <string>
#include <vector>
#include <set>
#include <functional>

class ShaderPreprocessor {
public:
    // Callback for logging errors/warnings during preprocessing
    std::function<void(const std::string&)> onMessage;

    ShaderPreprocessor();

    // Preprocesses a shader file, resolving #pragma include directives.
    // Returns the preprocessed source code. If errors occur (e.g., include loop, file not found),
    // the returned string will contain #error directives.
    // The 'includedFiles' vector will be populated with all unique files included (including the root).
    std::string preprocess(const std::string& filePath, std::vector<std::string>& includedFiles);

private:
    // Recursive helper for preprocessing
    std::string preprocessRecursive(const std::string& filePath,
                                    std::vector<std::string>& includeStack,
                                    std::set<std::string>& uniqueIncludedFiles);
};