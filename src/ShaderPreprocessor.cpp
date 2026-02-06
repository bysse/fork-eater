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
    std::string currentGroup = "";
    result.source = preprocessRecursive(filePath, includeStack, uniqueIncludedFiles, result.switchFlags, result.sliders, result.uniformRanges, result.labels, result.lineMappings, result.groupChanges, currentGroup, currentLine);

    for (const auto& file : uniqueIncludedFiles) {
        result.includedFiles.push_back(file);
    }
    
    // Conditional Chunk Logic Injection
    if ((scaleMode == RenderScaleMode::Chunk || scaleMode == RenderScaleMode::Auto) && filePath.find(".frag") != std::string::npos) {
        std::string chunkUniforms = R"(
// Chunk rendering uniforms
uniform bool u_progressive_fill;
uniform int u_render_phase;
uniform float u_renderChunkFactor;
uniform float u_time_offset;
uniform int u_chunk_stride;

bool shouldDiscard() {
    if (!u_progressive_fill) return false;
    // Use 2x2 pixel blocks to preserve quad efficiency (derivatives)
    // dividing gl_FragCoord by 2 ensures that a 2x2 pixel quad falls into the same "chunk"
    ivec2 coord = ivec2(gl_FragCoord.xy) / 2;
    // Stride-based stipple pattern for variable density
    int phase = (coord.x % u_chunk_stride) + (coord.y % u_chunk_stride) * u_chunk_stride;
    return phase != u_render_phase;
}
)";
        
        // Insert uniforms and helper function after version directive
        int insertedLines = std::count(chunkUniforms.begin(), chunkUniforms.end(), '\n');
        size_t versionPos = result.source.find("#version");
        int insertionLine = 1;

        if (versionPos != std::string::npos) {
            size_t eolPos = result.source.find('\n', versionPos);
            if (eolPos != std::string::npos) {
                result.source.insert(eolPos + 1, chunkUniforms);
                insertionLine = 2; // Inserted after line 1
            }
        } else {
            result.source.insert(0, chunkUniforms);
            insertionLine = 1;
        }

        // Shift metadata after the injection point
        auto shiftMetadata = [&](int fromLine, int delta) {
            for (auto& range : result.uniformRanges) {
                if (range.line >= fromLine) range.line += delta;
            }
            for (auto& change : result.groupChanges) {
                if (change.line >= fromLine) change.line += delta;
            }
            for (auto& mapping : result.lineMappings) {
                if (mapping.preprocessedLine >= fromLine) mapping.preprocessedLine += delta;
            }
        };

        shiftMetadata(insertionLine, insertedLines);
        
        // Insert discard check at start of main
        std::regex mainRegex(R"(void\s+main\s*\(\s*\)\s*\{)");
        std::smatch matches;
        if (std::regex_search(result.source, matches, mainRegex)) {
            size_t mainPos = matches.position() + matches.length();
            
            // Find which line main is on to shift mappings after it
            int mainLine = std::count(result.source.begin(), result.source.begin() + mainPos, '\n') + 1;
            
            std::string discardLogic = "\n    if (shouldDiscard()) discard;\n";
            int discardLines = std::count(discardLogic.begin(), discardLogic.end(), '\n');
            
            result.source.insert(mainPos, discardLogic);
            shiftMetadata(mainLine + 1, discardLines);
        }
    }
    
    return result;
}

std::string ShaderPreprocessor::preprocessRecursive(const std::string& filePath,
                                                    std::vector<std::string>& includeStack,
                                                    std::set<std::string>& uniqueIncludedFiles,
                                                    std::vector<SwitchInfo>& switchFlags,
                                                    std::vector<SliderInfo>& sliders,
                                                    std::vector<UniformRange>& uniformRanges,
                                                    std::vector<LabelInfo>& labels,
                                                    std::vector<LineMapping>& lineMappings,
                                                    std::vector<GroupChange>& groupChanges,
                                                    std::string& currentGroup,
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

    std::string result = preprocessSource(source, filePath, includeStack, uniqueIncludedFiles, switchFlags, sliders, uniformRanges, labels, lineMappings, groupChanges, currentGroup, currentLine);

    includeStack.pop_back();
    return result;
}

std::string ShaderPreprocessor::preprocessSource(const std::string& source,
                                                 const std::string& filePath,
                                                 std::vector<std::string>& includeStack,
                                                 std::set<std::string>& uniqueIncludedFiles,
                                                 std::vector<SwitchInfo>& switchFlags,
                                                 std::vector<SliderInfo>& sliders,
                                                 std::vector<UniformRange>& uniformRanges,
                                                 std::vector<LabelInfo>& labels,
                                                 std::vector<LineMapping>& lineMappings,
                                                 std::vector<GroupChange>& groupChanges,
                                                 std::string& currentGroup,
                                                 int& currentLine) {
    LOG_DEBUG("Processing source for: {}", filePath);

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
    if (!versionDirective.empty()) {
        preprocessedSource << versionDirective;
        // The version directive takes up one line (it includes the newline)
        // We only increment if this is the top-level file (currentLine == 1)
        // because sub-files shouldn't have #version anyway, but if they do,
        // it might be stripped or handled differently.
        // Actually, preprocessRecursive sets currentLine.
        if (currentLine == 1) {
            lineMappings.push_back({currentLine++, filePath, 1}); // Map #version line
        }
    }
    
    ss.clear();
    ss.str(content.str());
    
    std::regex includeRegex(R"x(#pragma\s+include\s*(?:\(\s*)?(?:<([^>]+)>|"([^"]+)"|([^\s\)"<]+))(?:\s*\))?)x");
    // #pragma switch(NAME [, defaultValue] [, "offLabel" [, "onLabel"]])
    std::regex switchRegex(R"x(#pragma\s+switch\s*\(\s*([a-zA-Z0-9_]+)\s*(?:,\s*(true|false|on|off))?\s*(?:,\s*["']([^"']+)["'])?\s*(?:,\s*["']([^"']+)["'])?\s*\))x");
    // #pragma slider(NAME, min, max [, "Label"])
    std::regex sliderRegex(R"x(#pragma\s+slider\s*\(\s*([a-zA-Z0-9_]+)\s*,\s*([-+]?[0-9]+)\s*,\s*([-+]?[0-9]+)\s*(?:,\s*["']([^"']+)["'])?\s*\))x");
    // #pragma range(NAME, min, max [, "Label"])
    std::regex rangeRegex(R"x(#pragma\s+range\s*\(\s*([a-zA-Z0-9_]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*(?:,\s*["']([^"']+)["'])?\s*\))x");
    // #pragma range(min, max [, "Label"])
    std::regex rangePositionalRegex(R"x(#pragma\s+range\s*\(\s*([-+]?[0-9]*\.?[0-9]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*(?:,\s*["']([^"']+)["'])?\s*\))x");
    std::regex labelRegex(R"x(#pragma\s+label\s*\(\s*([a-zA-Z0-9_]+)\s*,\s*["']([^"']+)["']\s*\))x");
    std::regex groupRegex(R"x(#pragma\s+group\s*\(\s*["']([^"']+)["']\s*\))x");
    std::regex endGroupRegex(R"x(#pragma\s+endgroup\s*(?:\(\s*\))?)x");

    int fileLineNumber = 0;
    
    while (std::getline(ss, line)) {
        ++fileLineNumber;
        std::smatch matches;

        // Scan for group start
        if (std::regex_search(line, matches, groupRegex)) {
            if (matches.size() >= 2) {
                currentGroup = matches[1].str();
                groupChanges.push_back({currentLine, currentGroup});
            }
        }
        
        // Scan for group end
        if (std::regex_search(line, matches, endGroupRegex)) {
            currentGroup = "";
            groupChanges.push_back({currentLine, ""});
        }

        // Scan for all switches on the line
        auto switchBegin = std::sregex_iterator(line.begin(), line.end(), switchRegex);
        auto switchEnd = std::sregex_iterator();
        
        for (std::sregex_iterator i = switchBegin; i != switchEnd; ++i) {
            std::smatch match = *i;
            if (match.size() >= 2) {
                SwitchInfo sw;
                sw.name = match[1].str();
                
                if (match.size() >= 3 && !match[2].str().empty()) {
                    std::string defaultStr = match[2].str();
                    sw.defaultValue = (defaultStr == "true" || defaultStr == "on");
                }
                
                if (match.size() >= 4 && !match[3].str().empty()) {
                    sw.label = match[3].str();
                }

                if (match.size() >= 5 && !match[4].str().empty()) {
                    sw.labelOn = match[4].str();
                }
                
                sw.group = currentGroup;
                switchFlags.push_back(sw);
            }
        }

        // Scan for sliders
        auto sliderBegin = std::sregex_iterator(line.begin(), line.end(), sliderRegex);
        for (std::sregex_iterator i = sliderBegin; i != switchEnd; ++i) {
            std::smatch match = *i;
            if (match.size() >= 4) {
                SliderInfo sl;
                sl.name = match[1].str();
                sl.min = std::stoi(match[2].str());
                sl.max = std::stoi(match[3].str());
                if (match.size() >= 5 && !match[4].str().empty()) {
                    sl.label = match[4].str();
                }
                sl.group = currentGroup;
                sliders.push_back(sl);
            }
        }

        // Scan for named ranges
        auto rangeBegin = std::sregex_iterator(line.begin(), line.end(), rangeRegex);
        for (std::sregex_iterator i = rangeBegin; i != switchEnd; ++i) {
            std::smatch match = *i;
            if (match.size() >= 4) {
                UniformRange r;
                r.name = match[1].str();
                r.min = std::stof(match[2].str());
                r.max = std::stof(match[3].str());
                if (match.size() >= 5 && !match[4].str().empty()) {
                    r.label = match[4].str();
                }
                uniformRanges.push_back(r);
            }
        }
        
        // Scan for positional ranges (min, max [, "label"])
        auto rangePosBegin = std::sregex_iterator(line.begin(), line.end(), rangePositionalRegex);
        for (std::sregex_iterator i = rangePosBegin; i != switchEnd; ++i) {
            std::smatch match = *i;
            if (match.size() >= 3) {
                UniformRange r;
                r.min = std::stof(match[1].str());
                r.max = std::stof(match[2].str());
                if (match.size() >= 4 && !match[3].str().empty()) {
                    r.label = match[3].str();
                }
                r.line = currentLine;
                uniformRanges.push_back(r);
            }
        }

        // Scan for labels
        auto labelBegin = std::sregex_iterator(line.begin(), line.end(), labelRegex);
        for (std::sregex_iterator i = labelBegin; i != switchEnd; ++i) {
            std::smatch match = *i;
            if (match.size() == 3) {
                labels.push_back({match[1].str(), match[2].str()});
            }
        }

        if (std::regex_search(line, matches, includeRegex)) {
            if (matches.size() >= 4) {
                std::string libInclude = matches[1].str();
                std::string quoteInclude = matches[2].str();
                std::string bareInclude = matches[3].str();
                
                std::string includeFileName;
                if (!libInclude.empty()) includeFileName = libInclude;
                else if (!quoteInclude.empty()) includeFileName = quoteInclude;
                else includeFileName = bareInclude;
                
                std::string includedContent;
                
                // Check if it is an embedded library
                bool isEmbedded = !libInclude.empty();
                if (!isEmbedded && includeFileName.rfind("lib/", 0) == 0) {
                    isEmbedded = true;
                }

                bool foundInEmbedded = false;
                std::string libContent;

                if (isEmbedded) {
                     // Try exact match
                    auto it = EmbeddedLibraries::g_libs.find(includeFileName);
                    if (it != EmbeddedLibraries::g_libs.end()) {
                        libContent = std::string(it->second.first, it->second.second);
                        foundInEmbedded = true;
                    } else {
                         // Try stripping "lib/" or "libs/"
                         std::string strippedName = includeFileName;
                         if (strippedName.rfind("lib/", 0) == 0) strippedName = strippedName.substr(4);
                         else if (strippedName.rfind("libs/", 0) == 0) strippedName = strippedName.substr(5);
                         
                         it = EmbeddedLibraries::g_libs.find(strippedName);
                         if (it != EmbeddedLibraries::g_libs.end()) {
                            libContent = std::string(it->second.first, it->second.second);
                            foundInEmbedded = true;
                         }
                    }
                }

                if (foundInEmbedded) {
                    // Recursively process embedded content
                    std::string embeddedName = "embedded:" + includeFileName;
                    if (std::find(includeStack.begin(), includeStack.end(), embeddedName) != includeStack.end()) {
                        std::string errorMsg = "Include loop detected: " + embeddedName;
                        if (onMessage) onMessage(errorMsg);
                        includedContent = "#error " + errorMsg + "\n";
                    } else {
                        includeStack.push_back(embeddedName);
                        includedContent = preprocessSource(libContent, embeddedName, includeStack, uniqueIncludedFiles, switchFlags, sliders, uniformRanges, labels, lineMappings, groupChanges, currentGroup, currentLine);
                        includeStack.pop_back();
                    }
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
                    includedContent = preprocessRecursive(includePath, includeStack, uniqueIncludedFiles, switchFlags, sliders, uniformRanges, labels, lineMappings, groupChanges, currentGroup, currentLine);
                }
                preprocessedSource << includedContent;
            } else {
                std::string errorMsg = "Invalid include directive: " + line;
                if (onMessage) onMessage(errorMsg);
                preprocessedSource << "#error " + errorMsg + "\n";
                lineMappings.push_back({currentLine++, filePath, fileLineNumber});
            }
        } else if (std::sregex_iterator(line.begin(), line.end(), switchRegex) != switchEnd ||
                   std::sregex_iterator(line.begin(), line.end(), sliderRegex) != switchEnd ||
                   std::sregex_iterator(line.begin(), line.end(), rangeRegex) != switchEnd ||
                   std::sregex_iterator(line.begin(), line.end(), rangePositionalRegex) != switchEnd ||
                   std::sregex_iterator(line.begin(), line.end(), labelRegex) != switchEnd ||
                   std::regex_search(line, groupRegex) ||
                   std::regex_search(line, endGroupRegex)) {
            // Line contained pragmas that we parsed above, so we consume it.
        } else {
            preprocessedSource << line << "\n";
            lineMappings.push_back({currentLine++, filePath, fileLineNumber});
        }
    }

    return preprocessedSource.str();
}