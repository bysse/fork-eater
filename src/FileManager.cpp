#include "FileManager.h"
#include "ShaderManager.h"
#include "FileWatcher.h"
#include <fstream>
#include <iostream>
#include "Settings.h"
#include "RenderScaleMode.h"

FileManager::FileManager(std::shared_ptr<ShaderManager> shaderManager, 
                         std::shared_ptr<FileWatcher> fileWatcher)
    : m_shaderManager(shaderManager)
    , m_fileWatcher(fileWatcher)
    , m_autoReload(true) {
}

void FileManager::loadShaderFromFile(const std::string& shaderName) {
    auto shader = m_shaderManager->getShader(shaderName);
    if (!shader) return;
    
    std::ifstream vertFile(shader->vertexPath);
    if (vertFile.is_open()) {
        m_vertexShaderText.assign((std::istreambuf_iterator<char>(vertFile)),
                                  std::istreambuf_iterator<char>());
        vertFile.close();
    }
    
    std::ifstream fragFile(shader->fragmentPath);
    if (fragFile.is_open()) {
        m_fragmentShaderText.assign((std::istreambuf_iterator<char>(fragFile)),
                                   std::istreambuf_iterator<char>());
        fragFile.close();
    }
    
    // Set up file watching
    if (m_fileWatcher) {
        m_fileWatcher->addWatch(shader->vertexPath, 
            [this](const std::string& path) { onFileChanged(path); });
        m_fileWatcher->addWatch(shader->fragmentPath, 
            [this](const std::string& path) { onFileChanged(path); });
    }
}

void FileManager::saveShaderToFile(const std::string& shaderName) {
    auto shader = m_shaderManager->getShader(shaderName);
    if (!shader) return;
    
    std::ofstream vertFile(shader->vertexPath);
    if (vertFile.is_open()) {
        vertFile << m_vertexShaderText;
        vertFile.close();
    }
    
    std::ofstream fragFile(shader->fragmentPath);
    if (fragFile.is_open()) {
        fragFile << m_fragmentShaderText;
        fragFile.close();
    }
}

void FileManager::createNewShader() {
    // TODO: Implement new shader creation dialog
    static int counter = 1;
    std::string name = "shader_" + std::to_string(counter++);
    std::string vertPath = "shaders/" + name + ".vert";
    std::string fragPath = "shaders/" + name + ".frag";
    
    // Create default shader content
    m_vertexShaderText = "#version 330 core\nlayout (location = 0) in vec2 aPos;\nlayout (location = 1) in vec2 aTexCoord;\n\nout vec2 TexCoord;\n\nvoid main()\n{\n    gl_Position = vec4(aPos, 0.0, 1.0);\n    TexCoord = aTexCoord;\n}\n";
    m_fragmentShaderText = "#version 330 core\nout vec4 FragColor;\n\nin vec2 TexCoord;\nuniform float u_time;\nuniform vec2 u_resolution;\n\nvoid main()\n{\n    vec2 uv = TexCoord;\n    vec3 col = 0.5 + 0.5 * cos(u_time + uv.xyx + vec3(0, 2, 4));\n    FragColor = vec4(col*0.0, 1.0);\n}\n";
    
    // Save to files
    std::ofstream vertFile(vertPath);
    if (vertFile.is_open()) {
        vertFile << m_vertexShaderText;
        vertFile.close();
    }
    
    std::ofstream fragFile(fragPath);
    if (fragFile.is_open()) {
        fragFile << m_fragmentShaderText;
        fragFile.close();
    }
    
    // Load shader
    RenderScaleMode scaleMode = Settings::getInstance().getRenderScaleMode();
    m_shaderManager->loadShader(name, vertPath, fragPath, scaleMode);
}

void FileManager::onFileChanged(const std::string& filePath) {
    if (m_autoReload) {
        // Find shader that uses this file
        auto shaderNames = m_shaderManager->getShaderNames();
        for (const auto& name : shaderNames) {
            auto shader = m_shaderManager->getShader(name);
            if (shader && (shader->vertexPath == filePath || shader->fragmentPath == filePath)) {
                RenderScaleMode scaleMode = Settings::getInstance().getRenderScaleMode();
                m_shaderManager->reloadShader(name, scaleMode);
                break;
            }
        }
    }
}