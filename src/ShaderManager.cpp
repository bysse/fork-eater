#include "ShaderManager.h"
#include "ShaderPreprocessor.h"
#include "Logger.h"
#include "glad.h"
#include "imgui/imgui.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <regex> // Required for regex_search
#include <filesystem> // Required for path manipulation

// Helper function to read a file's content
static std::string readFileContent(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

ShaderManager::ShaderManager() : m_quadVAO(0), m_quadVBO(0), m_preprocessor(new ShaderPreprocessor()) {
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
    delete m_preprocessor;
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
    
    m_errorLogged[name] = false;
    auto shader = std::make_shared<ShaderProgram>();
    shader->vertexPath = vertexPath;
    shader->fragmentPath = fragmentPath;
    shader->isValid = false;

    auto vertexResult = m_preprocessor->preprocess(vertexPath);
    auto fragmentResult = m_preprocessor->preprocess(fragmentPath);

    shader->preprocessedVertexSource = vertexResult.source;
    shader->preprocessedFragmentSource = fragmentResult.source;
    shader->includedFiles = vertexResult.includedFiles;
    shader->includedFiles.insert(shader->includedFiles.end(), fragmentResult.includedFiles.begin(), fragmentResult.includedFiles.end());
    shader->switchFlags = vertexResult.switchFlags;
    shader->switchFlags.insert(shader->switchFlags.end(), fragmentResult.switchFlags.begin(), fragmentResult.switchFlags.end());

    // Combine and unique-ify included files
    std::sort(shader->includedFiles.begin(), shader->includedFiles.end());
    shader->includedFiles.erase(std::unique(shader->includedFiles.begin(), shader->includedFiles.end()), shader->includedFiles.end());
    
    if (vertexResult.source.empty() || fragmentResult.source.empty() || 
        vertexResult.source.find("#error") != std::string::npos || 
        fragmentResult.source.find("#error") != std::string::npos) {
        shader->lastError = "Failed to preprocess shader files or include error";
        if (m_compilationCallback) {
            m_compilationCallback(name, false, shader->lastError);
        }
        return shader;
    }
    
    // Compile vertex shader
    shader->vertexShaderId = compileShader(vertexResult.source, GL_VERTEX_SHADER);
    if (shader->vertexShaderId == 0) {
        shader->lastError = "Failed to compile vertex shader";
        if (m_compilationCallback) {
            m_compilationCallback(name, false, shader->lastError);
        }
        return shader;
    }
    
    // Compile fragment shader
    shader->fragmentShaderId = compileShader(fragmentResult.source, GL_FRAGMENT_SHADER);
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

    // Parse uniforms
    std::regex uniformRegex("uniform\\s+(float|vec2|vec3|vec4)\\s+([a-zA-Z0-9_]+);");
    std::string shaderCode = shader->preprocessedFragmentSource;
    std::smatch matches;
    auto it = shaderCode.cbegin();
    while (std::regex_search(it, shaderCode.cend(), matches, uniformRegex)) {
        if (matches.size() == 3) {
            std::string typeStr = matches[1].str();
            std::string nameStr = matches[2].str();

            // Skip system uniforms
            if (nameStr == "u_time" || nameStr == "u_resolution" || nameStr == "u_mouse" ||
                nameStr == "iTime" || nameStr == "iResolution" || nameStr == "iMouse") {
                it = matches[0].second;
                continue;
            }

            ShaderUniform uniform;
            uniform.name = nameStr;
            if (typeStr == "float") {
                uniform.type = GL_FLOAT;
            } else if (typeStr == "vec2") {
                uniform.type = GL_FLOAT_VEC2;
            } else if (typeStr == "vec3") {
                uniform.type = GL_FLOAT_VEC3;
            } else if (typeStr == "vec4") {
                uniform.type = GL_FLOAT_VEC4;
            }
            shader->uniforms.push_back(uniform);
        }
        it = matches[0].second;
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
    
    m_errorLogged[name] = false;
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
        if (!m_errorLogged[name]) {
            LOG_ERROR("[ShaderManager] Failed to use shader: {} (not found or invalid)", name);
            m_errorLogged[name] = true;
        }
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

std::string ShaderManager::getCurrentShader() const {
    return m_currentShader;
}

void ShaderManager::setCompilationCallback(std::function<void(const std::string&, bool, const std::string&)> callback) {
    m_compilationCallback = callback;
}

std::string ShaderManager::getPreprocessedSource(const std::string& name, bool fragment) {
    auto shader = getShader(name);
    if (shader) {
        return fragment ? shader->preprocessedFragmentSource : shader->preprocessedVertexSource;
    }
    return "";
}

bool ShaderManager::getSwitchState(const std::string& name) const {
    auto it = m_switchStates.find(name);
    if (it != m_switchStates.end()) {
        return it->second;
    }
    return false;
}

void ShaderManager::setSwitchState(const std::string& name, bool enabled) {
    m_switchStates[name] = enabled;
}

const std::unordered_map<std::string, bool>& ShaderManager::getSwitchStates() const {
    return m_switchStates;
}

GLuint ShaderManager::compileShader(const std::string& source, GLenum shaderType) {
    std::string finalSource = source;
    for (const auto& [name, enabled] : m_switchStates) {
        if (enabled) {
            size_t versionPos = finalSource.find("#version");
            if (versionPos != std::string::npos) {
                size_t eolPos = finalSource.find('\n', versionPos);
                if (eolPos != std::string::npos) {
                    finalSource.insert(eolPos + 1, "#define " + name + "\n");
                }
            }
        }
    }

    GLuint shader = glCreateShader(shaderType);
    
    const char* sourcePtr = finalSource.c_str();
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

void ShaderManager::renderToFramebuffer(const std::string& name, int width, int height, float time, float renderScaleFactor) {
    int scaledWidth = static_cast<int>(width * renderScaleFactor);
    int scaledHeight = static_cast<int>(height * renderScaleFactor);

    auto it = m_framebuffers.find(name);
    if (it == m_framebuffers.end()) {
        m_framebuffers[name] = std::make_unique<Framebuffer>(scaledWidth, scaledHeight);
    } else if (it->second->getWidth() != scaledWidth || it->second->getHeight() != scaledHeight) {
        // Resize framebuffer if dimensions changed
        it->second->resize(scaledWidth, scaledHeight);
    }

    m_framebuffers[name]->bind();
    glViewport(0, 0, scaledWidth, scaledHeight); // Set viewport to scaled dimensions
    useShader(name);
    setUniform("u_time", time);
    setUniform("iTime", time);
    float resolution[3] = {(float)scaledWidth, (float)scaledHeight, (float)scaledWidth / (float)scaledHeight};
    setUniform("u_resolution", resolution, 2);
    setUniform("iResolution", resolution, 3);

    auto shader = getShader(name);
    if (shader) {
        for (const auto& uniform : shader->uniforms) {
            GLint location = glGetUniformLocation(shader->programId, uniform.name.c_str());
            if (location != -1) {
                switch (uniform.type) {
                    case GL_FLOAT:
                        glUniform1f(location, uniform.value[0]);
                        break;
                    case GL_FLOAT_VEC2:
                        glUniform2fv(location, 1, uniform.value);
                        break;
                    case GL_FLOAT_VEC3:
                        glUniform3fv(location, 1, uniform.value);
                        break;
                    case GL_FLOAT_VEC4:
                        glUniform4fv(location, 1, uniform.value);
                        break;
                }
            }
        }
    }

    ImGuiIO& io = ImGui::GetIO();
    float mouse[4] = {
        io.MousePos.x / (float)width,
        io.MousePos.y / (float)height,
        io.MouseDown[0] ? 1.0f : 0.0f,
        0.0f
    };
    setUniform("iMouse", mouse, 4);
    setUniform("u_mouse", mouse, 4);

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    m_framebuffers[name]->unbind();
    glViewport(0, 0, width, height); // Restore viewport to original dimensions
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
