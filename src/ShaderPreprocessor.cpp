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

ShaderPreprocessor::PreprocessResult ShaderPreprocessor::preprocess(const std::string& filePath, RenderScaleMode scaleMode) {
    PreprocessResult result;
    std::vector<std::string> includeStack;
    std::set<std::string> uniqueIncludedFiles;
    int currentLine = 1;
    result.source = preprocessRecursive(filePath, includeStack, uniqueIncludedFiles, result.switchFlags, result.lineMappings, currentLine);

    for (const auto& file : uniqueIncludedFiles) {
        result.includedFiles.push_back(file);
    }
    
    // Conditional Chunk Logic Injection
    if (scaleMode == RenderScaleMode::Chunk && filePath.find(".frag") != std::string::npos) {
        std::string chunkUniforms = R"(
// Chunk rendering uniforms
uniform bool u_progressive_fill;
uniform int u_render_phase;
uniform float u_renderChunkFactor;
uniform float u_time_offset;

bool shouldDiscard() {
    if (!u_progressive_fill) return false;
    ivec2 coord = ivec2(gl_FragCoord.xy);
    // 2x2 Bayer matrix pattern for 4 phases
    int phase = (coord.x % 2) + (coord.y % 2) * 2;
    return phase != u_render_phase;
}
)";
        
        // Insert uniforms and helper function after version directive
        size_t versionPos = result.source.find("#version");
        if (versionPos != std::string::npos) {
            size_t eolPos = result.source.find('\n', versionPos);
            if (eolPos != std::string::npos) {
                result.source.insert(eolPos + 1, chunkUniforms);
            }
        } else {
            result.source.insert(0, chunkUniforms);
        }
        
        // Insert discard check at start of main
        std::regex mainRegex(R"(void\s+main\s*\(\s*\)\s*\{)");
        std::smatch matches;
        if (std::regex_search(result.source, matches, mainRegex)) {
            size_t mainPos = matches.position() + matches.length();
            result.source.insert(mainPos, "\n    if (shouldDiscard()) discard;\n");
        }
    }
    
    return result;
}

std::string ShaderPreprocessor::preprocessRecursive(const std::string& filePath,
                                                    std::vector<std::string>& includeStack,
                                                    std::set<std::string>& uniqueIncludedFiles,
                                                    std::vector<std::string>& switchFlags,
                                                    std::vector<LineMapping>& lineMappings,
                                                    int& currentLine) {
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
    std::string versionDirective;
    std::stringstream content;
    std::stringstream ss(source);
    std::string line;
    std::regex versionRegex(R"x(\s*#\s*version\s+\d+\s+\w*)x");

    // First, separate the #version directive from the rest of the content
    while (std::getline(ss, line)) {
        if (std::regex_match(line, versionRegex) && versionDirective.empty()) {
            versionDirective = line + "\n";
        } else {
            content << line << "\n";
        }
    }

    // Now, process the content for includes and other pragmas
    preprocessedSource << versionDirective;
    ss.clear();
    ss.str(content.str());
    
    std::regex includeRegex(R"x(#pragma\s+include\s*(?:<([^>]+)>|"([^"]+)"))x");
    std::regex switchRegex(R"x(#pragma\s+switch\s*\(([^)]+)\))x");
    int fileLineNumber = 0;
    
    while (std::getline(ss, line)) {
        ++fileLineNumber;
        std::smatch matches;
        if (std::regex_search(line, matches, includeRegex)) {
            if (matches.size() == 3) {
                std::string libInclude = matches[1].str();
                std::string fileInclude = matches[2].str();
                std::string includeFileName = !libInclude.empty() ? libInclude : fileInclude;
                
                std::string includedContent;
                
                // Check if it is an embedded library
                bool isEmbedded = !libInclude.empty();
                if (!isEmbedded && includeFileName.rfind("lib/", 0) == 0) {
                    isEmbedded = true;
                    // Strip "lib/" prefix for lookup if needed, but for now try exact match first
                    // If we assume libs/utils.glsl is stored as "utils.glsl", we should strip.
                    // But if it is stored as "lib/utils.glsl", we shouldn't.
                    // Let's try to lookup exact first, then stripped.
                }

                bool foundInEmbedded = false;
                if (isEmbedded) {
                     // Try exact match
                    auto it = EmbeddedLibraries::g_libs.find(includeFileName);
                    if (it != EmbeddedLibraries::g_libs.end()) {
                        std::string libContent(it->second.first, it->second.second);
                        includedContent = libContent;
                        foundInEmbedded = true;
                    } else {
                         // Try stripping "lib/" or "libs/"
                         std::string strippedName = includeFileName;
                         if (strippedName.rfind("lib/", 0) == 0) strippedName = strippedName.substr(4);
                         else if (strippedName.rfind("libs/", 0) == 0) strippedName = strippedName.substr(5);
                         
                         it = EmbeddedLibraries::g_libs.find(strippedName);
                         if (it != EmbeddedLibraries::g_libs.end()) {
                            std::string libContent(it->second.first, it->second.second);
                            includedContent = libContent;
                            foundInEmbedded = true;
                         }
                    }
                }

                if (foundInEmbedded) {
                    // Already loaded into includedContent
                } else if (!libInclude.empty()) {
                    // explicit <lib> but not found
                    std::string errorMsg = "Embedded library not found: " + includeFileName;
                    if (onMessage) onMessage(errorMsg);
                    preprocessedSource << "#error " + errorMsg + "\n";
                    lineMappings.push_back({currentLine++, filePath, fileLineNumber});
                    continue;
                } else {
                    // Filesystem include
                    std::filesystem::path currentDirPath = std::filesystem::path(filePath).parent_path();
                    std::string includePath = (currentDirPath / includeFileName).string();
                    includedContent = preprocessRecursive(includePath, includeStack, uniqueIncludedFiles, switchFlags, lineMappings, currentLine);
                }
                preprocessedSource << includedContent;
            } else {
                std::string errorMsg = "Invalid include directive: " + line;
                if (onMessage) onMessage(errorMsg);
                preprocessedSource << "#error " + errorMsg + "\n";
                lineMappings.push_back({currentLine++, filePath, fileLineNumber});
            }
        } else if (std::regex_search(line, matches, switchRegex)) {
            if (matches.size() == 2) {
                std::string switchName = matches[1].str();
                switchFlags.push_back(switchName);
            } else {
                std::string errorMsg = "Invalid switch directive: " + line;
                if (onMessage) onMessage(errorMsg);
                preprocessedSource << "#error " + errorMsg + "\n";
                lineMappings.push_back({currentLine++, filePath, fileLineNumber});
            }
        } else {
            preprocessedSource << line << "\n";
            lineMappings.push_back({currentLine++, filePath, fileLineNumber});
        }
    }

    includeStack.pop_back();
    return preprocessedSource.str();
}
