#pragma once

#include <memory>
#include <string>
#include <vector>

// Forward declare OpenGL types
typedef unsigned int GLuint;

// Forward declare ImGui types
struct ImVec2;

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
    bool m_showLeftPanel;
    bool m_showErrorPanel;
    float m_leftPanelWidth;
    float m_errorPanelHeight;
    
    // Render settings
    enum class AspectMode {
        Free,
        Fixed_16_9,
        Fixed_4_3,
        Fixed_1_1,
        Fixed_21_9
    };
    
    AspectMode m_aspectMode;
    
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
    void renderRenderMenu();
    void renderMainLayout();
    void renderLeftPanel();
    void renderPassList();
    void renderFileList();
    void renderPreviewPanel();
    void renderErrorPanel();
    void setupPreviewQuad();
    void cleanupPreview();
    
    // Helper methods
    ImVec2 calculatePreviewSize(ImVec2 availableSize);
    void onShaderCompiled(const std::string& name, bool success, const std::string& error);
    void onFileChanged(const std::string& filePath);
    void loadShaderFromFile(const std::string& shaderName);
    void saveShaderToFile(const std::string& shaderName);
    void createNewShader();
};