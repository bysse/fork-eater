#pragma once

#include <memory>
#include <string>

// Forward declare OpenGL types
typedef unsigned int GLuint;

// Forward declare ImGui types
struct ImVec2;

class ShaderManager;

enum class AspectMode {
    Free,
    Fixed_16_9,
    Fixed_4_3,
    Fixed_1_1,
    Fixed_21_9
};

class PreviewPanel {
public:
    PreviewPanel(std::shared_ptr<ShaderManager> shaderManager);
    ~PreviewPanel();
    
    // Initialize OpenGL resources
    bool initialize();
    
    // Render the preview panel
    void render(const std::string& selectedShader, float time);
    
    // Aspect ratio settings
    AspectMode getAspectMode() const { return m_aspectMode; }
    void setAspectMode(AspectMode mode) { m_aspectMode = mode; }
    
private:
    std::shared_ptr<ShaderManager> m_shaderManager;
    
    // Aspect ratio settings
    AspectMode m_aspectMode;
    
    // Shader uniforms
    float m_resolution[2];
    
    // OpenGL resources
    GLuint m_previewVAO;
    GLuint m_previewVBO;
    GLuint m_previewFramebuffer;
    GLuint m_previewTexture;
    
    // Private methods
    void renderPreviewPanel(const std::string& selectedShader, float time);
    ImVec2 calculatePreviewSize(ImVec2 availableSize);
    void setupPreviewQuad();
    void cleanupPreview();
    void setupFramebuffer(int width, int height);
};