#include "ShaderProject.h"
#include "ShaderManager.h"
#include "ShaderTemplates.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include "Logger.h"
#include <algorithm>
#include <unordered_set>
#include "json.hpp"

using json = nlohmann::json;

namespace fs = std::filesystem;

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
    
    for (const auto& pass : m_manifest.passes) {
        if (!pass.enabled) continue;
        
        std::string vertPath = getShaderPath(pass.vertexShader);
        std::string fragPath = getShaderPath(pass.fragmentShader);
        
        auto shader = shaderManager->loadShader(pass.name, vertPath, fragPath);
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
