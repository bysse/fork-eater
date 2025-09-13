#pragma once

#include <memory>
#include <string>

class ShaderManager;
class FileWatcher;

class FileManager {
public:
    FileManager(std::shared_ptr<ShaderManager> shaderManager, 
                std::shared_ptr<FileWatcher> fileWatcher);
    ~FileManager() = default;
    
    // File operations
    void loadShaderFromFile(const std::string& shaderName);
    void saveShaderToFile(const std::string& shaderName);
    void createNewShader();
    void onFileChanged(const std::string& filePath);
    
    // Getters for shader text (for external editing)
    const std::string& getVertexShaderText() const { return m_vertexShaderText; }
    const std::string& getFragmentShaderText() const { return m_fragmentShaderText; }
    
    // Auto-reload setting
    void setAutoReload(bool autoReload) { m_autoReload = autoReload; }
    bool isAutoReloadEnabled() const { return m_autoReload; }
    
private:
    std::shared_ptr<ShaderManager> m_shaderManager;
    std::shared_ptr<FileWatcher> m_fileWatcher;
    
    // Shader text (kept for compatibility, but not used for editing in this refactor)
    std::string m_vertexShaderText;
    std::string m_fragmentShaderText;
    
    // Settings
    bool m_autoReload;
};