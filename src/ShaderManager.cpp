#include "ShaderManager.h"
#include "Logger.h"
#include "glad.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

ShaderManager::ShaderManager() : m_quadVAO(0), m_quadVBO(0) {
    // Full-screen quad vertices
    float vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
        
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

ShaderManager::~ShaderManager() {
    for (auto& pair : m_shaders) {
        cleanupShader(*pair.second);
    }
    glDeleteVertexArrays(1, &m_quadVAO);
    glDeleteBuffers(1, &m_quadVBO);
}

std::shared_ptr<ShaderManager::ShaderProgram> ShaderManager::loadShader(
    const std::string& name, 
    const std::string& vertexPath, 
    const std::string& fragmentPath) {
    
    LOG_DEBUG("[ShaderManager] Loading shader '{}': {} + {}", name, vertexPath, fragmentPath);
    
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
        // Shader is active, reduce log spam
        if (m_currentShader != name) {
            LOG_DEBUG("[ShaderManager] Switching to shader: {}", name);
        }

        glUseProgram(shader->programId);
        m_currentShader = name;
    } else {
        LOG_ERROR("[ShaderManager] Failed to use shader: {} (not found or invalid)", name);
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
        LOG_ERROR("Shader compilation failed: {}", errorLog);
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
        LOG_ERROR("Shader linking failed: {}", errorLog);
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}

std::string ShaderManager::readFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file: {}", filePath);
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

void ShaderManager::renderToFramebuffer(const std::string& name, int width, int height, float time) {
    auto it = m_framebuffers.find(name);
    if (it == m_framebuffers.end()) {
        m_framebuffers[name] = std::make_unique<Framebuffer>(width, height);
    }

    m_framebuffers[name]->bind();
    useShader(name);
    setUniform("u_time", time);
    float resolution[2] = {(float)width, (float)height};
    setUniform("u_resolution", resolution, 2);

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    m_framebuffers[name]->unbind();
}

GLuint ShaderManager::getFramebufferTexture(const std::string& name) {
    auto it = m_framebuffers.find(name);
    if (it != m_framebuffers.end()) {
        return it->second->getTextureId();
    }
    return 0;
}

void ShaderManager::clearShaders() {
    // Clean up all shader programs
    for (auto& [name, shader] : m_shaders) {
        if (shader) {
            cleanupShader(*shader);
        }
    }
    
    // Clear the map
    m_shaders.clear();
    m_currentShader.clear();
    m_framebuffers.clear();
    
    LOG_INFO("Cleared all loaded shaders");
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
