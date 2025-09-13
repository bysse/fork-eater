#pragma once

#include <memory>
#include <string>
#include <vector>

// Forward declare OpenGL types
typedef unsigned int GLuint;

class ShaderManager;
class FileWatcher;

class ShaderEditor {
public:
    ShaderEditor(std::shared_ptr<ShaderManager> shaderManager, 
                 std::shared_ptr<FileWatcher> fileWatcher);
    ~ShaderEditor();
    
    // Initialize the editor
    bool initialize();
    
    // Render the editor GUI
    void render();
    
    // Handle window events
    void handleResize(int width, int height);
    
    // Check if application should exit
    bool shouldExit() const;

private:
    std::shared_ptr<ShaderManager> m_shaderManager;
    std::shared_ptr<FileWatcher> m_fileWatcher;
    
    // GUI state
    bool m_showDemo;
    bool m_showShaderList;
    bool m_showShaderEditor;
    bool m_showCompileLog;
    bool m_showPreview;
    
    // Editor state
    std::string m_selectedShader;
    std::string m_vertexShaderText;
    std::string m_fragmentShaderText;
    std::string m_compileLog;
    bool m_autoReload;
    bool m_exitRequested;
    
    // Preview state
    float m_time;
    float m_resolution[2];
    GLuint m_previewVAO;
    GLuint m_previewVBO;
    GLuint m_previewFramebuffer;
    GLuint m_previewTexture;
    
    // Private methods
    void renderMenuBar();
    void renderShaderList();
    void renderShaderEditor();
    void renderCompileLog();
    void renderPreview();
    void setupPreviewQuad();
    void cleanupPreview();
    void onShaderCompiled(const std::string& name, bool success, const std::string& error);
    void onFileChanged(const std::string& filePath);
    void loadShaderFromFile(const std::string& shaderName);
    void saveShaderToFile(const std::string& shaderName);
    void createNewShader();
};