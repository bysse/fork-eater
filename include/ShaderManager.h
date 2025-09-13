#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <vector>

// Forward declare OpenGL types
typedef unsigned int GLuint;
typedef unsigned int GLenum;

class ShaderManager {
public:
    struct ShaderProgram {
        GLuint programId;
        GLuint vertexShaderId;
        GLuint fragmentShaderId;
        std::string vertexPath;
        std::string fragmentPath;
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
    
    // Set uniform helpers
    void setUniform(const std::string& name, float value);
    void setUniform(const std::string& name, const float* value, int count);
    void setUniform(const std::string& name, int value);
    void setUniform(const std::string& name, bool value);
    
    // Get all shader names
    std::vector<std::string> getShaderNames() const;
    
    // Set compilation callback
    void setCompilationCallback(std::function<void(const std::string&, bool, const std::string&)> callback);

private:
    std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> m_shaders;
    std::string m_currentShader;
    std::function<void(const std::string&, bool, const std::string&)> m_compilationCallback;
    
    // Helper functions
    GLuint compileShader(const std::string& source, GLenum shaderType);
    GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader);
    std::string readFile(const std::string& filePath);
    std::string getShaderInfoLog(GLuint shader);
    std::string getProgramInfoLog(GLuint program);
    void cleanupShader(ShaderProgram& shader);
};