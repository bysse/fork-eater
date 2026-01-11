#include "ShaderManager.h"
#include "ShaderPreprocessor.h"
#include "Logger.h"
#include "glad.h"
#include "imgui/imgui.h"
#include "Settings.h"
#include "RenderScaleMode.h"
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
    const std::string& fragmentPath,
    RenderScaleMode scaleMode) {
    
    LOG_DEBUG("[ShaderManager] Loading shader '{}': {} + {}", name, vertexPath, fragmentPath);
    
    m_errorLogged[name] = false;
    auto shader = std::make_shared<ShaderProgram>();
    shader->vertexPath = vertexPath;
    shader->fragmentPath = fragmentPath;
    shader->isValid = false;

    auto vertexResult = m_preprocessor->preprocess(vertexPath, scaleMode);
    auto fragmentResult = m_preprocessor->preprocess(fragmentPath, scaleMode);

    shader->preprocessedVertexSource = vertexResult.source;
    shader->preprocessedFragmentSource = fragmentResult.source;
    shader->vertexLineMappings = vertexResult.lineMappings;
    shader->fragmentLineMappings = fragmentResult.lineMappings;
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
    std::string vertexErrorLog;
    shader->vertexShaderId = compileShader(vertexResult.source, GL_VERTEX_SHADER, vertexErrorLog, &shader->vertexLineMappings);
    if (shader->vertexShaderId == 0) {
        shader->lastError = vertexErrorLog.empty() ? "Failed to compile vertex shader" : "Vertex shader compilation failed: " + vertexErrorLog;
        if (m_compilationCallback) {
            m_compilationCallback(name, false, shader->lastError);
        }
        return shader;
    }
    
    // Compile fragment shader
    std::string fragmentErrorLog;
    shader->fragmentShaderId = compileShader(fragmentResult.source, GL_FRAGMENT_SHADER, fragmentErrorLog, &shader->fragmentLineMappings);
    if (shader->fragmentShaderId == 0) {
        shader->lastError = fragmentErrorLog.empty() ? "Failed to compile fragment shader" : "Fragment shader compilation failed: " + fragmentErrorLog;
        cleanupShader(*shader);
        if (m_compilationCallback) {
            m_compilationCallback(name, false, shader->lastError);
        }
        return shader;
    }
    
    // Link program
    std::string linkErrorLog;
    shader->programId = linkProgram(shader->vertexShaderId, shader->fragmentShaderId, linkErrorLog);
    if (shader->programId == 0) {
        shader->lastError = linkErrorLog.empty() ? "Failed to link shader program" : "Shader linking failed: " + linkErrorLog;
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
                it = matches.suffix().first;
                continue;
            }
            
            GLenum type = GL_FLOAT;
            if (typeStr == "vec2") type = GL_FLOAT_VEC2;
            else if (typeStr == "vec3") type = GL_FLOAT_VEC3;
            else if (typeStr == "vec4") type = GL_FLOAT_VEC4;
            
            ShaderUniform uniform;
            uniform.name = nameStr;
            uniform.type = type;
            // Initialize with default values (0)
            uniform.value[0] = uniform.value[1] = uniform.value[2] = uniform.value[3] = 0.0f;
            
            // Try to find if this uniform already exists in the previous shader version to preserve value
            if (m_shaders.find(name) != m_shaders.end()) {
                auto& oldUniforms = m_shaders[name]->uniforms;
                for (const auto& oldU : oldUniforms) {
                    if (oldU.name == nameStr && oldU.type == type) {
                        for(int i=0; i<4; i++) uniform.value[i] = oldU.value[i];
                        break;
                    }
                }
            }
            
            shader->uniforms.push_back(uniform);
        }
        it = matches.suffix().first;
    }
    
    shader->isValid = true;
    m_shaders[name] = shader;
    
    if (m_compilationCallback) {
        m_compilationCallback(name, true, "");
    }
    
    return shader;
}

bool ShaderManager::reloadShader(const std::string& name, RenderScaleMode scaleMode) {
    auto it = m_shaders.find(name);
    if (it == m_shaders.end()) {
        return false;
    }
    
    m_errorLogged[name] = false;
    auto oldShader = it->second;
    auto newShader = loadShader(name, oldShader->vertexPath, oldShader->fragmentPath, scaleMode);
    
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

GLuint ShaderManager::compileShader(const std::string& source, GLenum shaderType, std::string& outErrorLog, const std::vector<ShaderPreprocessor::LineMapping>* lineMappings) {
    outErrorLog.clear();
    std::string finalSource = source;
    const char* stageName = (shaderType == GL_VERTEX_SHADER) ? "Vertex" :
                            (shaderType == GL_FRAGMENT_SHADER) ? "Fragment" : "Unknown";
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
        outErrorLog = getShaderInfoLog(shader);
        if (outErrorLog.empty()) {
            outErrorLog = "Shader compilation failed with an unknown error";
        }
        outErrorLog = remapErrorLog(outErrorLog, lineMappings);
        LOG_ERROR("{} shader compilation failed: {}", stageName, outErrorLog);
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

GLuint ShaderManager::linkProgram(GLuint vertexShader, GLuint fragmentShader, std::string& outErrorLog) {
    outErrorLog.clear();
    GLuint program = glCreateProgram();
    
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    
    if (!success) {
        outErrorLog = getProgramInfoLog(program);
        if (outErrorLog.empty()) {
            outErrorLog = "Shader linking failed with an unknown error";
        }
        LOG_ERROR("Shader linking failed: {}", outErrorLog);
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

std::string ShaderManager::remapErrorLog(const std::string& log, const std::vector<ShaderPreprocessor::LineMapping>* lineMappings) const {
    if (!lineMappings || lineMappings->empty() || log.empty()) {
        return log;
    }

    // Build quick lookup by preprocessed line (1-based)
    int maxLine = lineMappings->back().preprocessedLine;
    if (maxLine <= 0) {
        return log;
    }
    std::vector<ShaderPreprocessor::LineMapping> table(static_cast<size_t>(maxLine) + 1);
    std::vector<bool> has(static_cast<size_t>(maxLine) + 1, false);
    for (const auto& mapping : *lineMappings) {
        if (mapping.preprocessedLine > 0 && static_cast<size_t>(mapping.preprocessedLine) < table.size()) {
            table[static_cast<size_t>(mapping.preprocessedLine)] = mapping;
            has[static_cast<size_t>(mapping.preprocessedLine)] = true;
        }
    }

    std::stringstream input(log);
    std::stringstream output;
    std::string line;
    std::regex lineRegex(R"((\d+):(\d+)(?:\(\d+\))?)");
    bool first = true;

    while (std::getline(input, line)) {
        std::smatch match;
        std::string remappedLine = line;

        if (std::regex_search(line, match, lineRegex) && match.size() >= 3) {
            int preprocessedLine = 0;
            try {
                preprocessedLine = std::stoi(match[2].str());
            } catch (...) {
                preprocessedLine = 0;
            }

            const ShaderPreprocessor::LineMapping* mappingPtr = nullptr;
            auto lookup = [&](int lineNumber) -> const ShaderPreprocessor::LineMapping* {
                if (lineNumber > 0 && static_cast<size_t>(lineNumber) < has.size() && has[static_cast<size_t>(lineNumber)]) {
                    return &table[static_cast<size_t>(lineNumber)];
                }
                return nullptr;
            };

            mappingPtr = lookup(preprocessedLine);
            if (!mappingPtr && preprocessedLine > 1) {
                // Try off-by-one correction since some drivers report 0-based lines
                mappingPtr = lookup(preprocessedLine - 1);
            }

            if (mappingPtr && !mappingPtr->filePath.empty()) {
                remappedLine = line + " [at " + mappingPtr->filePath + ":" + std::to_string(mappingPtr->fileLine) + "]";
            }
        }

        if (!first) {
            output << "\n";
        }
        output << remappedLine;
        first = false;
    }

    return output.str();
}

void ShaderManager::renderToFramebuffer(const std::string& name, int width, int height, float time, float renderScaleFactor) {
    RenderScaleMode scaleMode = Settings::getInstance().getRenderScaleMode();
    
    int scaledWidth, scaledHeight;
    bool chunkMode = (scaleMode == RenderScaleMode::Chunk);

    if (chunkMode) {
        // In chunk mode, we maintain full resolution but render sparsely
        scaledWidth = width;
        scaledHeight = height;
    } else {
        // In resolution mode, we scale the framebuffer dimensions
        scaledWidth = static_cast<int>(width * renderScaleFactor);
        scaledHeight = static_cast<int>(height * renderScaleFactor);
    }

    auto it = m_framebuffers.find(name);
    if (it == m_framebuffers.end()) {
        m_framebuffers[name] = std::make_unique<Framebuffer>(scaledWidth, scaledHeight);
    } else if (it->second->getWidth() != scaledWidth || it->second->getHeight() != scaledHeight) {
        // Resize framebuffer if dimensions changed
        it->second->resize(scaledWidth, scaledHeight);
    }

    // Set texture filtering
    // Always use LINEAR filtering for smoother results when scaling
    m_framebuffers[name]->setFilter(GL_LINEAR);

    m_framebuffers[name]->bind();
    glViewport(0, 0, scaledWidth, scaledHeight); // Set viewport to scaled dimensions
    useShader(name);
    setUniform("u_time", time);
    setUniform("iTime", time);
    float resolution[3] = {(float)scaledWidth, (float)scaledHeight, (float)scaledWidth / (float)scaledHeight};
    setUniform("u_resolution", resolution, 2);
    setUniform("iResolution", resolution, 3);
    
    // Chunk Rendering Uniforms
    if (chunkMode) {
        setUniform("u_progressive_fill", true);
        
        // Calculate stride based on render scale factor
        // Stride = 1 / scale. E.g. 0.5 scale -> stride 2. 0.1 scale -> stride 10.
        int stride = std::max(1, static_cast<int>(1.0f / renderScaleFactor));
        setUniform("u_chunk_stride", stride);
        
        // Use ImGui frame count for phase synchronization
        int totalPhases = stride * stride;
        int frameCount = ImGui::GetFrameCount();
        int phase = frameCount % totalPhases; 
        setUniform("u_render_phase", phase);
        
        // Optional: pass the chunk factor if we want to support variable sparsity later
        // For now, it's hardcoded to 2x2 in the injection, effectively 0.25 density
        setUniform("u_renderChunkFactor", renderScaleFactor); 
        setUniform("u_time_offset", 0.0f); // Could be used for temporal dithering
    } else {
        setUniform("u_progressive_fill", false);
    }

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
    bool mouseDown = io.MouseDown[0];
    if (mouseDown) {
        m_mouseUniform[0] = io.MousePos.x / (float)width;
        m_mouseUniform[1] = io.MousePos.y / (float)height;
    }
    m_mouseUniform[2] = mouseDown ? 1.0f : 0.0f;
    m_mouseUniform[3] = 0.0f;
    setUniform("iMouse", m_mouseUniform, 4);
    setUniform("u_mouse", m_mouseUniform, 4);

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
