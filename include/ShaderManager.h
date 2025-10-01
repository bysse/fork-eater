#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <vector>

// Forward declare OpenGL types
typedef unsigned int GLuint;
typedef unsigned int GLenum;

#include "Framebuffer.h"

class ShaderPreprocessor;

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
        std::vector<std::string> includedFiles;
        std::string lastError;
        bool isValid;
    };

    ShaderManager();
    ~ShaderManager();

    // Load and compile shader program
    std::shared_ptr<ShaderProgram> loadShader(const std::string& name, 
                                               const std::string& vertexPath, 
                                               const std::string& fragmentPath);
    
    // Reload existing shader
    bool reloadShader(const std::string& name);
    
    // Get shader program
    std::shared_ptr<ShaderProgram> getShader(const std::string& name);
    
    // Use shader program
    void useShader(const std::string& name);
    
    // Render to framebuffer
    void renderToFramebuffer(const std::string& name, int width, int height, float time, float renderScaleFactor);

    // Get texture ID of a framebuffer
    GLuint getFramebufferTexture(const std::string& name);

    // Set uniform helpers
    void setUniform(const std::string& name, float value);
    void setUniform(const std::string& name, const float* value, int count);
    void setUniform(const std::string& name, int value);
    void setUniform(const std::string& name, bool value);
    
    // Get all shader names
    std::vector<std::string> getShaderNames() const;
    
    // Clear all loaded shaders
    void clearShaders();
    
    // Set compilation callback
    void setCompilationCallback(std::function<void(const std::string&, bool, const std::string&)> callback);

    // Get preprocessed shader source
    std::string getPreprocessedSource(const std::string& name, bool fragment = true);

private:
    std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> m_shaders;
    std::unordered_map<std::string, std::unique_ptr<Framebuffer>> m_framebuffers;
    std::string m_currentShader;
    std::function<void(const std::string&, bool, const std::string&)> m_compilationCallback;
    GLuint m_quadVAO;
    GLuint m_quadVBO;
    
    // Helper functions
    GLuint compileShader(const std::string& source, GLenum shaderType);
    GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader);
    std::string readFile(const std::string& filePath);
    std::string getShaderInfoLog(GLuint shader);
    std::string getProgramInfoLog(GLuint program);
    void cleanupShader(ShaderProgram& shader);
    
    ShaderPreprocessor* m_preprocessor;
};