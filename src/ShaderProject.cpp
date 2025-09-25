#include "ShaderProject.h"
#include "ShaderManager.h"
#include "ShaderTemplates.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <algorithm>

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
        std::cerr << "Project directory does not exist: " << projectPath << std::endl;
        return false;
    }
    
    if (!loadManifest()) {
        std::cerr << "Failed to load manifest from: " << projectPath << std::endl;
        return false;
    }
    
    if (!validateProject()) {
        std::cerr << "Project validation failed for: " << projectPath << std::endl;
        return false;
    }
    
    m_isLoaded = true;
    std::cout << "Successfully loaded shader project: " << m_manifest.name << std::endl;
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
        std::cerr << "Template not found: " << templateName << ". Using default template." << std::endl;
        shaderTemplate = templateManager.getDefaultTemplate();
        if (!shaderTemplate) {
            std::cerr << "No default template available!" << std::endl;
            return false;
        }
    }
    
    // Parse manifest from template
    if (!parseManifestJson(shaderTemplate->manifestJson)) {
        std::cerr << "Failed to parse template manifest" << std::endl;
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
        std::cerr << "Manifest file not found: " << manifestPath << std::endl;
        return false;
    }
    
    std::ifstream file(manifestPath);
    if (!file.is_open()) {
        std::cerr << "Cannot open manifest file: " << manifestPath << std::endl;
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
        std::cerr << "Cannot create manifest file: " << manifestPath << std::endl;
        return false;
    }
    
    std::string jsonContent = generateManifestJson();
    file << jsonContent;
    return true;
}

bool ShaderProject::parseManifestJson(const std::string& jsonContent) {
    // Simple JSON parser for the manifest format
    // For production, you'd want to use a proper JSON library
    
    try {
        m_manifest = ShaderProjectManifest(); // Reset to defaults
        
        // Parse name
        size_t namePos = jsonContent.find("\"name\":");
        if (namePos != std::string::npos) {
            size_t start = jsonContent.find("\"", namePos + 7) + 1;
            size_t end = jsonContent.find("\"", start);
            if (start != std::string::npos && end != std::string::npos) {
                m_manifest.name = jsonContent.substr(start, end - start);
            }
        }
        
        // Parse description
        size_t descPos = jsonContent.find("\"description\":");
        if (descPos != std::string::npos) {
            size_t start = jsonContent.find("\"", descPos + 13) + 1;
            size_t end = jsonContent.find("\"", start);
            if (start != std::string::npos && end != std::string::npos) {
                m_manifest.description = jsonContent.substr(start, end - start);
            }
        }
        
        // Parse timeline length
        size_t lengthPos = jsonContent.find("\"timelineLength\":");
        if (lengthPos != std::string::npos) {
            size_t start = jsonContent.find_first_not_of(" \t", lengthPos + 17);
            size_t end = jsonContent.find_first_of(" ,\n\r\t}", start);
            if (start != std::string::npos && end != std::string::npos) {
                try {
                    m_manifest.timelineLength = std::stof(jsonContent.substr(start, end - start));
                } catch (...) {
                    m_manifest.timelineLength = 120.0f;
                }
            }
        }
        
        // Parse BPM
        size_t bpmPos = jsonContent.find("\"bpm\":");
        if (bpmPos != std::string::npos) {
            size_t start = jsonContent.find_first_not_of(" \t", bpmPos + 6);
            size_t end = jsonContent.find_first_of(" ,\n\r\t}", start);
            if (start != std::string::npos && end != std::string::npos) {
                try {
                    m_manifest.bpm = std::stof(jsonContent.substr(start, end - start));
                } catch (...) {
                    m_manifest.bpm = 120.0f;
                }
            }
        }
        
        // Parse beats per bar
        size_t beatsPos = jsonContent.find("\"beatsPerBar\":");
        if (beatsPos != std::string::npos) {
            size_t start = jsonContent.find_first_not_of(" \t", beatsPos + 14);
            size_t end = jsonContent.find_first_of(" ,\n\r\t}", start);
            if (start != std::string::npos && end != std::string::npos) {
                try {
                    m_manifest.beatsPerBar = std::stoi(jsonContent.substr(start, end - start));
                } catch (...) {
                    m_manifest.beatsPerBar = 4;
                }
            }
        }
        
        // Parse passes - simplified parsing for basic passes
        parsePassesFromJson(jsonContent);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing manifest JSON: " << e.what() << std::endl;
        return false;
    }
}

void ShaderProject::parsePassesFromJson(const std::string& jsonContent) {
    m_manifest.passes.clear();
    
    // Find the passes array
    size_t passesPos = jsonContent.find("\"passes\":");
    if (passesPos == std::string::npos) {
        // Create default pass if none found
        ShaderPass defaultPass;
        defaultPass.name = "main";
        defaultPass.vertexShader = "basic.vert";
        defaultPass.fragmentShader = "basic.frag";
        m_manifest.passes.push_back(defaultPass);
        return;
    }
    
    // Simple pass parsing - look for common patterns
    size_t arrayStart = jsonContent.find("[", passesPos);
    size_t arrayEnd = jsonContent.find("]", arrayStart);
    
    if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
        std::string passesSection = jsonContent.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
        
        // Parse individual passes (simplified)
        size_t pos = 0;
        while (pos < passesSection.length()) {
            size_t objStart = passesSection.find("{", pos);
            if (objStart == std::string::npos) break;
            
            size_t objEnd = passesSection.find("}", objStart);
            if (objEnd == std::string::npos) break;
            
            std::string passObj = passesSection.substr(objStart, objEnd - objStart + 1);
            ShaderPass pass = parsePassObject(passObj);
            m_manifest.passes.push_back(pass);
            
            pos = objEnd + 1;
        }
    }
    
    // Ensure at least one pass exists
    if (m_manifest.passes.empty()) {
        ShaderPass defaultPass;
        defaultPass.name = "main";
        defaultPass.vertexShader = "basic.vert";
        defaultPass.fragmentShader = "basic.frag";
        m_manifest.passes.push_back(defaultPass);
    }
}

ShaderPass ShaderProject::parsePassObject(const std::string& passJson) {
    ShaderPass pass;
    
    // Parse name
    size_t namePos = passJson.find("\"name\":");
    if (namePos != std::string::npos) {
        size_t start = passJson.find("\"", namePos + 7) + 1;
        size_t end = passJson.find("\"", start);
        if (start != std::string::npos && end != std::string::npos) {
            pass.name = passJson.substr(start, end - start);
        }
    }
    
    // Parse vertex shader
    size_t vertPos = passJson.find("\"vertexShader\":");
    if (vertPos != std::string::npos) {
        size_t start = passJson.find("\"", vertPos + 15) + 1;
        size_t end = passJson.find("\"", start);
        if (start != std::string::npos && end != std::string::npos) {
            pass.vertexShader = passJson.substr(start, end - start);
        }
    }
    
    // Parse fragment shader
    size_t fragPos = passJson.find("\"fragmentShader\":");
    if (fragPos != std::string::npos) {
        size_t start = passJson.find("\"", fragPos + 17) + 1;
        size_t end = passJson.find("\"", start);
        if (start != std::string::npos && end != std::string::npos) {
            pass.fragmentShader = passJson.substr(start, end - start);
        }
    }
    
    return pass;
}

std::string ShaderProject::generateManifestJson() const {
    std::ostringstream json;
    
    json << "{\n";
    json << "  \"name\": \"" << m_manifest.name << "\",\n";
    json << "  \"description\": \"" << m_manifest.description << "\",\n";
    json << "  \"version\": \"" << m_manifest.version << "\",\n";
    json << "  \"timelineLength\": " << m_manifest.timelineLength << ",\n";
    json << "  \"bpm\": " << m_manifest.bpm << ",\n";
    json << "  \"beatsPerBar\": " << m_manifest.beatsPerBar << ",\n";
    json << "  \"passes\": [\n";
    
    for (size_t i = 0; i < m_manifest.passes.size(); ++i) {
        const auto& pass = m_manifest.passes[i];
        json << "    {\n";
        json << "      \"name\": \"" << pass.name << "\",\n";
        json << "      \"vertexShader\": \"" << pass.vertexShader << "\",\n";
        json << "      \"fragmentShader\": \"" << pass.fragmentShader << "\",\n";
        json << "      \"enabled\": " << (pass.enabled ? "true" : "false") << "\n";
        json << "    }";
        if (i < m_manifest.passes.size() - 1) {
            json << ",";
        }
        json << "\n";
    }
    
    json << "  ]\n";
    json << "}\n";
    
    return json.str();
}

bool ShaderProject::createDirectoryStructure() const {
    try {
        fs::create_directories(m_projectPath);
        fs::create_directories(getShaderPath(""));
        fs::create_directories(getAssetsPath());
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create directory structure: " << e.what() << std::endl;
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

bool ShaderProject::loadShadersIntoManager(std::shared_ptr<ShaderManager> shaderManager) const {
    if (!shaderManager || !m_isLoaded) {
        return false;
    }
    
    for (const auto& pass : m_manifest.passes) {
        if (!pass.enabled) continue;
        
        std::string vertPath = getShaderPath(pass.vertexShader);
        std::string fragPath = getShaderPath(pass.fragmentShader);
        
        if (!shaderManager->loadShader(pass.name, vertPath, fragPath)) {
            std::cerr << "Failed to load shader pass: " << pass.name << std::endl;
            return false;
        }
    }
    
    return true;
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
    // Get the first pass to determine shader filenames
    if (m_manifest.passes.empty()) {
        std::cerr << "No passes defined in manifest" << std::endl;
        return false;
    }
    
    const auto& firstPass = m_manifest.passes[0];
    
    // Create vertex shader
    if (!firstPass.vertexShader.empty()) {
        std::string vertexPath = getShaderPath(firstPass.vertexShader);
        std::ofstream vertFile(vertexPath);
        if (!vertFile.is_open()) {
            std::cerr << "Failed to create vertex shader file: " << vertexPath << std::endl;
            return false;
        }
        vertFile << shaderTemplate.vertexShader;
        vertFile.close();
    }
    
    // Create fragment shader
    if (!firstPass.fragmentShader.empty()) {
        std::string fragmentPath = getShaderPath(firstPass.fragmentShader);
        std::ofstream fragFile(fragmentPath);
        if (!fragFile.is_open()) {
            std::cerr << "Failed to create fragment shader file: " << fragmentPath << std::endl;
            return false;
        }
        fragFile << shaderTemplate.fragmentShader;
        fragFile.close();
    }
    
    return true;
}
