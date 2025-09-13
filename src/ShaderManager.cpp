#include "ShaderManager.h"
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

// Function pointers for OpenGL extensions
PFNGLCREATESHADERPROC glCreateShader = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC glDeleteShader = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
PFNGLATTACHSHADERPROC glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
PFNGLUNIFORM1FPROC glUniform1f = nullptr;
PFNGLUNIFORM1IPROC glUniform1i = nullptr;
PFNGLUNIFORM2FVPROC glUniform2fv = nullptr;
PFNGLUNIFORM3FVPROC glUniform3fv = nullptr;
PFNGLUNIFORM4FVPROC glUniform4fv = nullptr;

void loadGLExtensions() {
    glCreateShader = (PFNGLCREATESHADERPROC)glfwGetProcAddress("glCreateShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)glfwGetProcAddress("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)glfwGetProcAddress("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)glfwGetProcAddress("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)glfwGetProcAddress("glGetShaderInfoLog");
    glDeleteShader = (PFNGLDELETESHADERPROC)glfwGetProcAddress("glDeleteShader");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)glfwGetProcAddress("glCreateProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)glfwGetProcAddress("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)glfwGetProcAddress("glLinkProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)glfwGetProcAddress("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)glfwGetProcAddress("glGetProgramInfoLog");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)glfwGetProcAddress("glDeleteProgram");
    glUseProgram = (PFNGLUSEPROGRAMPROC)glfwGetProcAddress("glUseProgram");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)glfwGetProcAddress("glGetUniformLocation");
    glUniform1f = (PFNGLUNIFORM1FPROC)glfwGetProcAddress("glUniform1f");
    glUniform1i = (PFNGLUNIFORM1IPROC)glfwGetProcAddress("glUniform1i");
    glUniform2fv = (PFNGLUNIFORM2FVPROC)glfwGetProcAddress("glUniform2fv");
    glUniform3fv = (PFNGLUNIFORM3FVPROC)glfwGetProcAddress("glUniform3fv");
    glUniform4fv = (PFNGLUNIFORM4FVPROC)glfwGetProcAddress("glUniform4fv");
}

ShaderManager::ShaderManager() {
    loadGLExtensions();
}

ShaderManager::~ShaderManager() {
    for (auto& pair : m_shaders) {
        cleanupShader(*pair.second);
    }
}

std::shared_ptr<ShaderManager::ShaderProgram> ShaderManager::loadShader(
    const std::string& name, 
    const std::string& vertexPath, 
    const std::string& fragmentPath) {
    
    auto shader = std::make_shared<ShaderProgram>();
    shader->vertexPath = vertexPath;
    shader->fragmentPath = fragmentPath;
    shader->isValid = false;
    
    // Read shader sources
    std::string vertexSource = readFile(vertexPath);
    std::string fragmentSource = readFile(fragmentPath);
    
    if (vertexSource.empty() || fragmentSource.empty()) {
        shader->lastError = "Failed to read shader files";
        if (m_compilationCallback) {
            m_compilationCallback(name, false, shader->lastError);
        }
        return shader;
    }
    
    // Compile vertex shader
    shader->vertexShaderId = compileShader(vertexSource, GL_VERTEX_SHADER);
    if (shader->vertexShaderId == 0) {
        shader->lastError = "Failed to compile vertex shader";
        if (m_compilationCallback) {
            m_compilationCallback(name, false, shader->lastError);
        }
        return shader;
    }
    
    // Compile fragment shader
    shader->fragmentShaderId = compileShader(fragmentSource, GL_FRAGMENT_SHADER);
    if (shader->fragmentShaderId == 0) {
        shader->lastError = "Failed to compile fragment shader";
        cleanupShader(*shader);
        if (m_compilationCallback) {
            m_compilationCallback(name, false, shader->lastError);
        }
        return shader;
    }
    
    // Link program
    shader->programId = linkProgram(shader->vertexShaderId, shader->fragmentShaderId);
    if (shader->programId == 0) {
        shader->lastError = "Failed to link shader program";
        cleanupShader(*shader);
        if (m_compilationCallback) {
            m_compilationCallback(name, false, shader->lastError);
        }
        return shader;
    }
    
    shader->isValid = true;
    shader->lastError = "Compilation successful";
    m_shaders[name] = shader;
    
    if (m_compilationCallback) {
        m_compilationCallback(name, true, shader->lastError);
    }
    
    return shader;
}

bool ShaderManager::reloadShader(const std::string& name) {
    auto it = m_shaders.find(name);
    if (it == m_shaders.end()) {
        return false;
    }
    
    auto oldShader = it->second;
    auto newShader = loadShader(name, oldShader->vertexPath, oldShader->fragmentPath);
    
    return newShader->isValid;
}

std::shared_ptr<ShaderManager::ShaderProgram> ShaderManager::getShader(const std::string& name) {
    auto it = m_shaders.find(name);
    return (it != m_shaders.end()) ? it->second : nullptr;
}

void ShaderManager::useShader(const std::string& name) {
    auto shader = getShader(name);
    if (shader && shader->isValid) {
        glUseProgram(shader->programId);
        m_currentShader = name;
    }
}

void ShaderManager::setUniform(const std::string& name, float value) {
    if (m_currentShader.empty()) return;
    
    auto shader = getShader(m_currentShader);
    if (shader && shader->isValid) {
        GLint location = glGetUniformLocation(shader->programId, name.c_str());
        if (location != -1) {
            glUniform1f(location, value);
        }
    }
}

void ShaderManager::setUniform(const std::string& name, const float* value, int count) {
    if (m_currentShader.empty()) return;
    
    auto shader = getShader(m_currentShader);
    if (shader && shader->isValid) {
        GLint location = glGetUniformLocation(shader->programId, name.c_str());
        if (location != -1) {
            if (count == 2) glUniform2fv(location, 1, value);
            else if (count == 3) glUniform3fv(location, 1, value);
            else if (count == 4) glUniform4fv(location, 1, value);
        }
    }
}

void ShaderManager::setUniform(const std::string& name, int value) {
    if (m_currentShader.empty()) return;
    
    auto shader = getShader(m_currentShader);
    if (shader && shader->isValid) {
        GLint location = glGetUniformLocation(shader->programId, name.c_str());
        if (location != -1) {
            glUniform1i(location, value);
        }
    }
}

void ShaderManager::setUniform(const std::string& name, bool value) {
    setUniform(name, value ? 1 : 0);
}

std::vector<std::string> ShaderManager::getShaderNames() const {
    std::vector<std::string> names;
    for (const auto& pair : m_shaders) {
        names.push_back(pair.first);
    }
    return names;
}

void ShaderManager::setCompilationCallback(std::function<void(const std::string&, bool, const std::string&)> callback) {
    m_compilationCallback = callback;
}

GLuint ShaderManager::compileShader(const std::string& source, GLenum shaderType) {
    GLuint shader = glCreateShader(shaderType);
    
    const char* sourcePtr = source.c_str();
    glShaderSource(shader, 1, &sourcePtr, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        std::string errorLog = getShaderInfoLog(shader);
        std::cerr << "Shader compilation failed: " << errorLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

GLuint ShaderManager::linkProgram(GLuint vertexShader, GLuint fragmentShader) {
    GLuint program = glCreateProgram();
    
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    
    if (!success) {
        std::string errorLog = getProgramInfoLog(program);
        std::cerr << "Shader linking failed: " << errorLog << std::endl;
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}

std::string ShaderManager::readFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string ShaderManager::getShaderInfoLog(GLuint shader) {
    GLint logLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    
    if (logLength <= 0) {
        return "";
    }
    
    std::vector<char> log(logLength);
    glGetShaderInfoLog(shader, logLength, nullptr, log.data());
    
    return std::string(log.data());
}

std::string ShaderManager::getProgramInfoLog(GLuint program) {
    GLint logLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    
    if (logLength <= 0) {
        return "";
    }
    
    std::vector<char> log(logLength);
    glGetProgramInfoLog(program, logLength, nullptr, log.data());
    
    return std::string(log.data());
}

void ShaderManager::cleanupShader(ShaderProgram& shader) {
    if (shader.programId != 0) {
        glDeleteProgram(shader.programId);
        shader.programId = 0;
    }
    
    if (shader.vertexShaderId != 0) {
        glDeleteShader(shader.vertexShaderId);
        shader.vertexShaderId = 0;
    }
    
    if (shader.fragmentShaderId != 0) {
        glDeleteShader(shader.fragmentShaderId);
        shader.fragmentShaderId = 0;
    }
    
    shader.isValid = false;
}