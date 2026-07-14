#include "ShaderProject.h"
#include "ShaderManager.h"
#include <cstring>
#include "ShaderTemplates.h"
#include "GeneratedShaderLibraries.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include "Logger.h"
#include <algorithm>
#include <unordered_set>
#include "json.hpp"
#include "Settings.h"
#include "RenderScaleMode.h"

using json = nlohmann::json;

namespace fs = std::filesystem;

static float quantizeFloat(float value, int mantissaBits) {
    if (mantissaBits >= 23 || mantissaBits < 0) {
        return value;
    }
    uint32_t valInt;
    std::memcpy(&valInt, &value, sizeof(float));
    
    uint32_t sign = valInt & 0x80000000;
    uint32_t absVal = valInt & 0x7FFFFFFF;
    
    uint32_t exponent = absVal >> 23;
    if (exponent == 0xFF) {
        return value; // Keep Inf/NaN unchanged
    }
    
    int bitsToZero = 23 - mantissaBits;
    if (bitsToZero > 0) {
        // Round to nearest: add half quantum
        uint32_t halfQuantum = 1U << (bitsToZero - 1);
        if (exponent > 0) { // Only round normal numbers
            absVal += halfQuantum;
        }
        
        // Mask out the lower bits
        uint32_t mask = ~((1U << bitsToZero) - 1);
        absVal &= mask;
    }
    
    uint32_t quantizedInt = sign | absVal;
    float quantized;
    std::memcpy(&quantized, &quantizedInt, sizeof(float));
    return quantized;
}

static float quantizeFixedPoint(float value, int m, int n, bool isSigned) {
    if (std::isnan(value) || std::isinf(value)) {
        return value;
    }
    double scale = std::pow(2.0, n);
    double scaled = std::round(static_cast<double>(value) * scale);
    
    int shiftAmount = m + n;
    if (shiftAmount > 62) shiftAmount = 62;
    
    double minVal, maxVal;
    if (isSigned) {
        minVal = -static_cast<double>(1LL << shiftAmount);
        maxVal = static_cast<double>((1LL << shiftAmount) - 1);
    } else {
        minVal = 0.0;
        maxVal = static_cast<double>((1ULL << shiftAmount) - 1);
    }
    
    if (scaled < minVal) scaled = minVal;
    if (scaled > maxVal) scaled = maxVal;
    
    return static_cast<float>(scaled / scale);
}

ShaderProject::ShaderProject()
    : m_isLoaded(false) {
}

ShaderProject::~ShaderProject() {
    // Cleanup if needed
}

bool ShaderProject::loadFromDirectory(const std::string& projectPath) {
    m_projectPath = projectPath;
    m_isLoaded = false;
    
    if (!fs::exists(projectPath) || !fs::is_directory(projectPath)) {
        LOG_ERROR("Project directory does not exist: {}", projectPath);
        return false;
    }
    
    if (!loadManifest()) {
        LOG_ERROR("Failed to load manifest from: {}", projectPath);
        return false;
    }

    if (!validateProject()) {
        LOG_ERROR("Project validation failed for: {}", projectPath);
        return false;
    }
    
    m_isLoaded = true;
    LOG_IMPORTANT("Successfully loaded shader project: {}", m_manifest.name);

    // Ensure the libs folder is created and populated with libraries and metadata files
    exportLibraries();

    return true;
    }


bool ShaderProject::saveToDirectory(const std::string& projectPath) const {
    if (projectPath != m_projectPath) {
        // Save to different location - create directory structure
        const_cast<ShaderProject*>(this)->m_projectPath = projectPath;
        if (!createDirectoryStructure()) {
            return false;
        }
    }
    
    return saveManifest();
}

bool ShaderProject::createNew(const std::string& projectPath, const std::string& name, const std::string& templateName) {
    m_projectPath = projectPath;
    m_isLoaded = false;
    
    // Get template from template manager
    const auto& templateManager = ShaderTemplateManager::getInstance();
    const ShaderTemplate* shaderTemplate = templateManager.getTemplate(templateName);
    
    if (!shaderTemplate) {
        LOG_WARN("Template not found: {}. Using default template.", templateName);
        shaderTemplate = templateManager.getDefaultTemplate();
        if (!shaderTemplate) {
            LOG_ERROR("No default template available!");
            return false;
        }
    }
    
    if (shaderTemplate->manifestJson == nullptr || shaderTemplate->manifestJsonSize == 0) {
        LOG_ERROR("Template manifest is empty or missing: {}", templateName);
        return false;
    }

    // Parse manifest from template
    std::string manifestContent(shaderTemplate->manifestJson, shaderTemplate->manifestJsonSize);
    if (!parseManifestJson(manifestContent)) {
        LOG_ERROR("Failed to parse template manifest");
        return false;
    }
    
    // Override name with provided name
    if (!name.empty()) {
        m_manifest.name = name;
    }
    
    if (!createDirectoryStructure()) {
        return false;
    }
    
    if (!saveManifest()) {
        return false;
    }
    
    // Create shader files from template
    if (!createShadersFromTemplate(*shaderTemplate)) {
        return false;
    }

    // Export bundled libraries to the project's libs folder
    exportLibraries();
    
    m_isLoaded = true;
    return true;
}

std::string ShaderProject::getManifestPath() const {
    return m_projectPath + "/" + SHADER_PROJECT_MANIFEST_FILENAME;
}

std::string ShaderProject::getShaderPath(const std::string& filename) const {
    return m_projectPath + "/shaders/" + filename;
}

std::string ShaderProject::getAssetsPath() const {
    return m_projectPath + "/assets";
}

bool ShaderProject::loadManifest() {
    std::string manifestPath = getManifestPath();
    
    if (!fs::exists(manifestPath)) {
        LOG_ERROR("Manifest file not found: {}", manifestPath);
        return false;
    }
    
    std::ifstream file(manifestPath);
    if (!file.is_open()) {
        LOG_ERROR("Cannot open manifest file: {}", manifestPath);
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    return parseManifestJson(content);
}

bool ShaderProject::saveManifest() const {
    std::string manifestPath = getManifestPath();
    
    std::ofstream file(manifestPath);
    if (!file.is_open()) {
        LOG_ERROR("Cannot create manifest file: {}", manifestPath);
        return false;
    }
    
    std::string jsonContent = generateManifestJson();
    file << jsonContent;
    return true;
}

bool ShaderProject::loadState(std::shared_ptr<ShaderManager> shaderManager) {
    std::string uniformsPath = m_projectPath + "/uniforms.json";
    if (!fs::exists(uniformsPath)) {
        return false;
    }

    std::ifstream file(uniformsPath);
    if (!file.is_open()) {
        LOG_ERROR("Cannot open uniforms file: {}", uniformsPath);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    try {
        json j = json::parse(content);
        if (j.contains("uniforms")) {
            m_uniformValues.clear();
            for (auto& [shaderName, uniforms] : j["uniforms"].items()) {
                for (auto& [uniformName, value] : uniforms.items()) {
                    m_uniformValues[shaderName][uniformName] = value.get<std::vector<float>>();
                }
            }
        }
        if (j.contains("switches")) {
            for (auto& [switchName, value] : j["switches"].items()) {
                shaderManager->setSwitchState(switchName, value.get<bool>());
            }
        }
        if (j.contains("sliders")) {
            for (auto& [sliderName, value] : j["sliders"].items()) {
                shaderManager->setSliderState(sliderName, value.get<int>());
            }
        }
    } catch (const json::parse_error& e) {
        LOG_ERROR("Error parsing uniforms JSON: {}", e.what());
        return false;
    }

    return true;
}

bool ShaderProject::saveState(std::shared_ptr<ShaderManager> shaderManager) const {
    std::string uniformsPath = m_projectPath + "/uniforms.json";
    std::ofstream file(uniformsPath);
    if (!file.is_open()) {
        LOG_ERROR("Cannot create uniforms file: {}", uniformsPath);
        return false;
    }

    json j;
    j["uniforms"] = m_uniformValues;
    j["switches"] = shaderManager->getSwitchStates();
    j["sliders"] = shaderManager->getSliderStates();
    file << j.dump(2);
    return true;
}

bool ShaderProject::parseManifestJson(const std::string& jsonContent) {
    try {
        json j = json::parse(jsonContent);
        
        m_manifest.name = j.value("name", "New Project");
        m_manifest.description = j.value("description", "");
        m_manifest.timelineLength = j.value("timelineLength", 120.0f);
        m_manifest.bpm = j.value("bpm", 120.0f);
        m_manifest.beatsPerBar = j.value("beatsPerBar", 4);
        m_manifest.audioFile = j.value("audio", "");
        
        m_manifest.passes.clear();
        if (j.contains("passes") && j["passes"].is_array()) {
            for (const auto& passJson : j["passes"]) {
                ShaderPass pass;
                pass.name = passJson.value("name", "main");
                pass.vertexShader = passJson.value("vertexShader", "");
                pass.fragmentShader = passJson.value("fragmentShader", "");
                pass.width = passJson.value("width", 0);
                pass.height = passJson.value("height", 0);
                pass.enabled = passJson.value("enabled", true);
                m_manifest.passes.push_back(pass);
            }
        }

        m_manifest.buffers.clear();
        if (j.contains("buffers") && j["buffers"].is_array()) {
            for (const auto& bufferJson : j["buffers"]) {
                ShaderBuffer buffer;
                buffer.name = bufferJson.value("name", "");
                std::string typeVal = bufferJson.value("type", "samplerBuffer");
                if (typeVal == "float" || typeVal == "vec2" || typeVal == "vec3" || typeVal == "vec4") {
                    buffer.dataType = typeVal;
                    buffer.type = "samplerBuffer";
                } else {
                    buffer.type = typeVal;
                    buffer.dataType = bufferJson.value("dataType", "float");
                }
                buffer.file = bufferJson.value("file", "");
                buffer.striped = bufferJson.value("striped", false);
                buffer.precision = bufferJson.value("precision", -1);
                
                if (bufferJson.contains("export") && bufferJson["export"].is_object()) {
                    auto exportJson = bufferJson["export"];
                    buffer.hasExport = true;
                    buffer.exportFormat = exportJson.value("format", "");
                    buffer.exportOutputFile = exportJson.value("outputFile", "");
                    buffer.exportStripedCoordinates = exportJson.value("stripedCoordinates", false);
                    buffer.exportStripedBytes = exportJson.value("stripedBytes", false);
                }
                
                std::string fpVal = bufferJson.value("fixedPoint", "");
                if (!fpVal.empty()) {
                    bool isSigned = true;
                    size_t startIdx = 0;
                    if (fpVal.rfind("UQ", 0) == 0) {
                        isSigned = false;
                        startIdx = 2;
                    } else if (fpVal.rfind("Q", 0) == 0) {
                        isSigned = true;
                        startIdx = 1;
                    } else {
                        LOG_ERROR("Invalid fixedPoint format: '{}'. Must start with 'Q' or 'UQ'.", fpVal);
                        return false;
                    }
                    
                    size_t dotIdx = fpVal.find('.', startIdx);
                    if (dotIdx == std::string::npos) {
                        LOG_ERROR("Invalid fixedPoint format: '{}'. Expected '.' separating integer and fractional bits (e.g. Q3.12).", fpVal);
                        return false;
                    }
                    
                    try {
                        int m = std::stoi(fpVal.substr(startIdx, dotIdx - startIdx));
                        int n = std::stoi(fpVal.substr(dotIdx + 1));
                        if (m < 0 || n < 0) {
                            LOG_ERROR("Invalid fixedPoint bit widths in format: '{}'. Widths must be non-negative.", fpVal);
                            return false;
                        }
                        buffer.fixedPoint = true;
                        buffer.fixedPointSigned = isSigned;
                        buffer.fixedPointM = m;
                        buffer.fixedPointN = n;
                        buffer.fixedPointFormat = fpVal;
                    } catch (const std::exception& e) {
                        LOG_ERROR("Error parsing fixedPoint format '{}': {}", fpVal, e.what());
                        return false;
                    }
                }
                
                if (bufferJson.contains("data") && bufferJson["data"].is_array()) {
                    buffer.data = bufferJson["data"].get<std::vector<float>>();
                }
                
                // If a file is specified, load it
                if (!buffer.file.empty()) {
                    fs::path filePath = fs::path(m_projectPath) / buffer.file;
                    filePath = filePath.lexically_normal();
                    if (fs::exists(filePath)) {
                        std::ifstream csvFile(filePath);
                        if (csvFile.is_open()) {
                            std::string line;
                            buffer.data.clear();
                            while (std::getline(csvFile, line)) {
                                if (line.empty() || line[0] == '#') continue;
                                // Simple CSV/space/tab parsing
                                std::replace(line.begin(), line.end(), ',', ' ');
                                std::stringstream ss(line);
                                float val;
                                while (ss >> val) {
                                    buffer.data.push_back(val);
                                }
                            }
                            LOG_DEBUG("Loaded {} values from buffer file: {}", buffer.data.size(), filePath.string());
                        } else {
                            LOG_ERROR("Failed to open buffer file: {}", filePath.string());
                        }
                    } else {
                        LOG_ERROR("Buffer file does not exist: {}", filePath.string());
                    }
                }
                
                if (buffer.precision >= 0) {
                    for (auto& val : buffer.data) {
                        val = quantizeFloat(val, buffer.precision);
                    }
                }
                
                if (buffer.fixedPoint) {
                    for (auto& val : buffer.data) {
                        val = quantizeFixedPoint(val, buffer.fixedPointM, buffer.fixedPointN, buffer.fixedPointSigned);
                    }
                }
                
                if (!buffer.name.empty()) {
                    m_manifest.buffers.push_back(buffer);
                }
            }
        }
        
        if (m_manifest.passes.empty()) {
            LOG_ERROR("No shader passes found in manifest");
            return false;
        }
        
        return true;
    } catch (const json::parse_error& e) {
        LOG_ERROR("Error parsing manifest JSON: {}", e.what());
        return false;
    }
}

std::string ShaderProject::generateManifestJson() const {
    json j;
    
    j["name"] = m_manifest.name;
    j["description"] = m_manifest.description;
    j["version"] = m_manifest.version;
    j["timelineLength"] = m_manifest.timelineLength;
    j["bpm"] = m_manifest.bpm;
    j["beatsPerBar"] = m_manifest.beatsPerBar;
    if (!m_manifest.audioFile.empty()) {
        j["audio"] = m_manifest.audioFile;
    }
    
    j["passes"] = json::array();
    for (const auto& pass : m_manifest.passes) {
        json passJson;
        passJson["name"] = pass.name;
        passJson["vertexShader"] = pass.vertexShader;
        passJson["fragmentShader"] = pass.fragmentShader;
        if (pass.width > 0) {
            passJson["width"] = pass.width;
        }
        if (pass.height > 0) {
            passJson["height"] = pass.height;
        }
        passJson["enabled"] = pass.enabled;
        j["passes"].push_back(passJson);
    }

    j["buffers"] = json::array();
    for (const auto& buffer : m_manifest.buffers) {
        json bufferJson;
        bufferJson["name"] = buffer.name;
        bufferJson["type"] = buffer.type;
        bufferJson["dataType"] = buffer.dataType;
        if (!buffer.file.empty()) {
            bufferJson["file"] = buffer.file;
        } else {
            bufferJson["data"] = buffer.data;
        }
        if (buffer.striped) {
            bufferJson["striped"] = true;
        }
        if (buffer.precision >= 0) {
            bufferJson["precision"] = buffer.precision;
        }
        if (buffer.fixedPoint) {
            bufferJson["fixedPoint"] = buffer.fixedPointFormat;
        }
        if (buffer.hasExport) {
            json exportJson;
            exportJson["format"] = buffer.exportFormat;
            exportJson["outputFile"] = buffer.exportOutputFile;
            exportJson["stripedCoordinates"] = buffer.exportStripedCoordinates;
            exportJson["stripedBytes"] = buffer.exportStripedBytes;
            bufferJson["export"] = exportJson;
        }
        j["buffers"].push_back(bufferJson);
    }
    
    return j.dump(2);
}

bool ShaderProject::createDirectoryStructure() const {
    try {
        fs::create_directories(m_projectPath);
        fs::create_directories(getShaderPath(""));
        fs::create_directories(getAssetsPath());
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to create directory structure: {}", e.what());
        return false;
    }
}

void ShaderProject::createDefaultShaders() const {
    // Create basic vertex shader
    std::string vertexPath = getShaderPath("basic.vert");
    std::ofstream vertFile(vertexPath);
    if (vertFile.is_open()) {
        vertFile << "#version 330 core\n";
        vertFile << "layout (location = 0) in vec2 aPos;\n";
        vertFile << "layout (location = 1) in vec2 aTexCoord;\n";
        vertFile << "out vec2 TexCoord;\n";
        vertFile << "void main() {\n";
        vertFile << "    gl_Position = vec4(aPos, 0.0, 1.0);\n";
        vertFile << "    TexCoord = aTexCoord;\n";
        vertFile << "}\n";
    }
    
    // Create basic fragment shader
    std::string fragmentPath = getShaderPath("basic.frag");
    std::ofstream fragFile(fragmentPath);
    if (fragFile.is_open()) {
        fragFile << "#version 330 core\n";
        fragFile << "uniform float u_time;\n";
        fragFile << "uniform vec2 u_resolution;\n";
        fragFile << "out vec4 FragColor;\n";
        fragFile << "void main() {\n";
        fragFile << "    vec2 uv = gl_FragCoord.xy / u_resolution.xy;\n";
        fragFile << "    vec3 col = 0.5 + 0.5 * cos(u_time + uv.xyx + vec3(0, 2, 4));\n";
        fragFile << "    FragColor = vec4(col*0.0, 1.0);\n";
        fragFile << "}\n";
    }
}

bool ShaderProject::validateProject() const {
    return validateManifest() && validateShaderFiles();
}

bool ShaderProject::validateManifest() const {
    if (m_manifest.name.empty()) return false;
    if (m_manifest.passes.empty()) return false;
    if (m_manifest.bpm <= 0) return false;
    if (m_manifest.timelineLength <= 0) return false;
    if (m_manifest.beatsPerBar <= 0) return false;
    
    return true;
}

bool ShaderProject::validateShaderFiles() const {
    for (const auto& pass : m_manifest.passes) {
        if (!pass.vertexShader.empty() && !fs::exists(getShaderPath(pass.vertexShader))) {
            return false;
        }
        if (!pass.fragmentShader.empty() && !fs::exists(getShaderPath(pass.fragmentShader))) {
            return false;
        }
    }
    return true;
}

std::vector<std::string> ShaderProject::getValidationErrors() const {
    std::vector<std::string> errors;
    
    if (m_manifest.name.empty()) {
        errors.push_back("Project name is empty");
    }
    if (m_manifest.passes.empty()) {
        errors.push_back("No shader passes defined");
    }
    if (m_manifest.bpm <= 0) {
        errors.push_back("Invalid BPM value");
    }
    if (m_manifest.timelineLength <= 0) {
        errors.push_back("Invalid timeline length");
    }
    
    for (const auto& pass : m_manifest.passes) {
        if (!pass.vertexShader.empty() && !fs::exists(getShaderPath(pass.vertexShader))) {
            errors.push_back("Vertex shader not found: " + pass.vertexShader);
        }
        if (!pass.fragmentShader.empty() && !fs::exists(getShaderPath(pass.fragmentShader))) {
            errors.push_back("Fragment shader not found: " + pass.fragmentShader);
        }
    }
    
    return errors;
}

bool ShaderProject::loadShadersIntoManager(std::shared_ptr<ShaderManager> shaderManager) {
    if (!shaderManager || !m_isLoaded) {
        return false;
    }
    
    RenderScaleMode scaleMode = Settings::getInstance().getRenderScaleMode();

    // Update data buffers first
    shaderManager->updateBuffers(m_manifest.buffers);

    for (const auto& pass : m_manifest.passes) {
        if (!pass.enabled) continue;
        
        std::string vertPath = getShaderPath(pass.vertexShader);
        std::string fragPath = getShaderPath(pass.fragmentShader);
        
        auto shader = shaderManager->loadShader(pass.name, vertPath, fragPath, scaleMode);
        if (!shader) {
            LOG_ERROR("Failed to load shader pass: {}", pass.name);
            return false;
        }

        // Apply saved uniform values
        applyUniformsToShader(pass.name, shader);
    }
    
    return true;
}

void ShaderProject::applyUniformsToShader(const std::string& passName, std::shared_ptr<ShaderManager::ShaderProgram> shader) {
    if (!shader) {
        return;
    }

    std::map<std::string, bool> uniformExistsInShader;

    for (auto& uniform : shader->uniforms) {
        uniformExistsInShader[uniform.name] = true;
        if (m_uniformValues.count(passName) && m_uniformValues.at(passName).count(uniform.name)) {
            // Uniform exists in project file, so apply saved value
            const auto& savedValue = m_uniformValues.at(passName).at(uniform.name);
            for (size_t i = 0; i < savedValue.size() && i < 4; ++i) {
                uniform.value[i] = savedValue[i];
            }
        } else {
            // Uniform is not in project file, so add it with default value
            std::vector<float> values(std::begin(uniform.value), std::end(uniform.value));
            m_uniformValues[passName][uniform.name] = values;
        }
    }

    // Clean up uniforms from project file that are no longer in the shader
    if (m_uniformValues.count(passName)) {
        auto& projectUniforms = m_uniformValues.at(passName);
        for (auto it = projectUniforms.begin(); it != projectUniforms.end(); ) {
            if (uniformExistsInShader.find(it->first) == uniformExistsInShader.end()) {
                it = projectUniforms.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void ShaderProject::addPass(const ShaderPass& pass) {
    m_manifest.passes.push_back(pass);
}

void ShaderProject::removePass(size_t index) {
    if (index < m_manifest.passes.size()) {
        m_manifest.passes.erase(m_manifest.passes.begin() + index);
    }
}

void ShaderProject::movePass(size_t from, size_t to) {
    if (from < m_manifest.passes.size() && to < m_manifest.passes.size() && from != to) {
        ShaderPass pass = m_manifest.passes[from];
        m_manifest.passes.erase(m_manifest.passes.begin() + from);
        m_manifest.passes.insert(m_manifest.passes.begin() + to, pass);
    }
}

bool ShaderProject::createShadersFromTemplate(const ShaderTemplate& shaderTemplate) const {
    if (m_manifest.passes.empty()) {
        LOG_ERROR("No passes defined in manifest");
        return false;
    }

    std::unordered_set<std::string> createdFiles;

    auto writeShaderFile = [&](const std::string& filename) -> bool {
        if (filename.empty() || createdFiles.count(filename) > 0) {
            return true;
        }

        auto fileIt = shaderTemplate.files.find(filename);
        if (fileIt == shaderTemplate.files.end()) {
            LOG_ERROR("Shader file '{}' not found in template '{}'", filename, shaderTemplate.name);
            return false;
        }

        std::string outputPath = getShaderPath(filename);
        std::ofstream outFile(outputPath, std::ios::binary);
        if (!outFile.is_open()) {
            LOG_ERROR("Failed to create shader file: {}", outputPath);
            return false;
        }
        outFile.write(fileIt->second.first, static_cast<std::streamsize>(fileIt->second.second));
        createdFiles.insert(filename);
        return true;
    };

    for (const auto& pass : m_manifest.passes) {
        if (!writeShaderFile(pass.vertexShader)) {
            return false;
        }
        if (!writeShaderFile(pass.fragmentShader)) {
            return false;
        }
    }

    return true;
}

bool ShaderProject::loadLocalState(LocalProjectState& state) const {
    std::string localPath = m_projectPath + "/" + SHADER_PROJECT_LOCAL_FILENAME;
    if (!fs::exists(localPath)) {
        return false;
    }

    std::ifstream file(localPath);
    if (!file.is_open()) {
        LOG_WARN("Cannot open local project file: {}", localPath);
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            if (key == "render_scale") {
                try {
                    state.renderScale = std::stof(value);
                } catch (...) {
                    LOG_WARN("Failed to parse render_scale from local state: {}", value);
                }
            } else if (key == "cam_pos") {
                try {
                    std::stringstream ss(value);
                    ss >> state.camPos[0] >> state.camPos[1] >> state.camPos[2];
                } catch (...) {
                    LOG_WARN("Failed to parse cam_pos from local state: {}", value);
                }
            } else if (key == "cam_target") {
                try {
                    std::stringstream ss(value);
                    ss >> state.camTarget[0] >> state.camTarget[1] >> state.camTarget[2];
                } catch (...) {
                    LOG_WARN("Failed to parse cam_target from local state: {}", value);
                }
            }
        }
    }
    return true;
}

bool ShaderProject::saveLocalState(const LocalProjectState& state) const {
    std::string localPath = m_projectPath + "/" + SHADER_PROJECT_LOCAL_FILENAME;
    std::ofstream file(localPath);
    if (!file.is_open()) {
        LOG_ERROR("Cannot create local project file: {}", localPath);
        return false;
    }

    file << "# Fork Eater Local Project Properties\n";
    file << "render_scale=" << state.renderScale << "\n";
    file << "cam_pos=" << state.camPos[0] << " " << state.camPos[1] << " " << state.camPos[2] << "\n";
    file << "cam_target=" << state.camTarget[0] << " " << state.camTarget[1] << " " << state.camTarget[2] << "\n";
    return true;
}

bool ShaderProject::exportLibraries() const {
    if (m_projectPath.empty()) {
        LOG_ERROR("Project path is empty, cannot export libraries");
        return false;
    }

    std::string libsDirPath = m_projectPath + "/libs";
    try {
        if (!fs::exists(libsDirPath)) {
            fs::create_directories(libsDirPath);
        }

        EmbeddedLibraries::initialize();
        
        LOG_INFO("Exporting {} bundled libraries to: {}", EmbeddedLibraries::g_libs.size(), libsDirPath);

        for (const auto& [name, content] : EmbeddedLibraries::g_libs) {
            std::string outputPath = libsDirPath + "/" + name;

            // Ensure subdirectories exist if the lib name contains them
            fs::path outPath(outputPath);
            if (outPath.has_parent_path()) {
                fs::create_directories(outPath.parent_path());
            }

            std::ofstream outFile(outputPath, std::ios::binary);
            if (!outFile.is_open()) {
                LOG_ERROR("Failed to create library file: {}", outputPath);
                continue;
            }
            outFile.write(content.first, static_cast<std::streamsize>(content.second));
            LOG_DEBUG("Exported: {}", name);
        }

        // Create .gitignore in the project root to ignore the libs folder
        std::string gitignorePath = m_projectPath + "/" + SHADER_PROJECT_GITIGNORE;
        std::ofstream gitignoreFile(gitignorePath);
        if (gitignoreFile.is_open()) {
            gitignoreFile << "libs/\n";
            LOG_DEBUG("Created: {}", SHADER_PROJECT_GITIGNORE);
        }

        // Create READ_ONLY file in the libs folder
        std::string readOnlyPath = libsDirPath + "/" + SHADER_PROJECT_LIBS_READ_ONLY;
        std::ofstream readOnlyFile(readOnlyPath);
        if (readOnlyFile.is_open()) {
            readOnlyFile << "This folder contains libraries bundled with Fork Eater.\n";
            readOnlyFile << "These files are automatically updated and should not be modified manually.\n";
            readOnlyFile << "Changes made to these files will be overwritten.\n";
            LOG_DEBUG("Created: {}", SHADER_PROJECT_LIBS_READ_ONLY);
        }

        LOG_IMPORTANT("Successfully exported bundled libraries to {}", libsDirPath);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to export libraries: {}", e.what());
        return false;
    }
}
