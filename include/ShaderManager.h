#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <vector>

#include "ShaderPreprocessor.h"
#include "RenderScaleMode.h"

// Forward declare OpenGL types
typedef unsigned int GLuint;
typedef unsigned int GLenum;

#include "Framebuffer.h"

class ShaderPreprocessor;

struct ShaderUniform {
    std::string name;
    GLenum type;
    float value[4];
};

class ShaderManager {
public:
    struct ShaderProgram {
        GLuint programId;
        GLuint vertexShaderId;
        GLuint fragmentShaderId;
        std::string vertexPath;
        std::string fragmentPath;
        std::string preprocessedVertexSource;
        std::string preprocessedFragmentSource;
        std::vector<ShaderPreprocessor::LineMapping> vertexLineMappings;
        std::vector<ShaderPreprocessor::LineMapping> fragmentLineMappings;
        std::vector<std::string> includedFiles;
        std::vector<ShaderUniform> uniforms;
        std::vector<std::string> switchFlags;
        std::string lastError;
        bool isValid;
    };

    ShaderManager();
    ~ShaderManager();

    // Load and compile shader program
    std::shared_ptr<ShaderProgram> loadShader(const std::string& name, 
                                               const std::string& vertexPath, 
                                               const std::string& fragmentPath,
                                               RenderScaleMode scaleMode = RenderScaleMode::Resolution);
    
    // Reload existing shader
    bool reloadShader(const std::string& name, RenderScaleMode scaleMode = RenderScaleMode::Resolution);
    
    // Get shader program
    std::shared_ptr<ShaderProgram> getShader(const std::string& name);
    
    // Use shader program
    void useShader(const std::string& name);
    
    // Render to framebuffer
    void renderToFramebuffer(const std::string& name, int width, int height, float time, float renderScaleFactor, RenderScaleMode scaleMode);

    // Get texture ID of a framebuffer
    GLuint getFramebufferTexture(const std::string& name);

    // Get the UV scale of the framebuffer texture (useful when rendering a sub-region)
    std::pair<float, float> getFramebufferUVScale(const std::string& name);

    // Set uniform helpers
    void setUniform(const std::string& name, float value);
    void setUniform(const std::string& name, const float* value, int count);
    void setUniform(const std::string& name, int value);
    void setUniform(const std::string& name, bool value);
    
    // Get all shader names
    std::vector<std::string> getShaderNames() const;

    // Get current shader name
    std::string getCurrentShader() const;
    
    // Clear all loaded shaders
    void clearShaders();
    
    // Set compilation callback
    void setCompilationCallback(std::function<void(const std::string&, bool, const std::string&)> callback);

    // Get preprocessed shader source
    std::string getPreprocessedSource(const std::string& name, bool fragment = true);

    // Switch state management
    bool getSwitchState(const std::string& name) const;
    void setSwitchState(const std::string& name, bool enabled);
    const std::unordered_map<std::string, bool>& getSwitchStates() const;

private:
    std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> m_shaders;
    std::unordered_map<std::string, std::unique_ptr<Framebuffer>> m_framebuffers;
    std::unordered_map<std::string, std::pair<float, float>> m_framebufferScales;
    std::string m_currentShader;
    std::function<void(const std::string&, bool, const std::string&)> m_compilationCallback;
    GLuint m_quadVAO;
    GLuint m_quadVBO;
    std::unordered_map<std::string, bool> m_errorLogged;
    std::unordered_map<std::string, bool> m_switchStates;
    float m_mouseUniform[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    
    // Helper functions
    GLuint compileShader(const std::string& source, GLenum shaderType, std::string& outErrorLog, const std::vector<ShaderPreprocessor::LineMapping>* lineMappings = nullptr);
    GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader, std::string& outErrorLog);
    std::string readFile(const std::string& filePath);
    std::string getShaderInfoLog(GLuint shader);
    std::string getProgramInfoLog(GLuint program);
    void cleanupShader(ShaderProgram& shader);
    std::string remapErrorLog(const std::string& log, const std::vector<ShaderPreprocessor::LineMapping>* lineMappings) const;
    
    ShaderPreprocessor* m_preprocessor;
};
