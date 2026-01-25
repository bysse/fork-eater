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

    setupSimpleTextureProgram();
}

ShaderManager::~ShaderManager() {
    delete m_preprocessor;
    for (auto& pair : m_shaders) {
        cleanupShader(*pair.second);
    }
    if (m_simpleTextureProgram.programId != 0) {
        glDeleteProgram(m_simpleTextureProgram.programId);
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
    shader->labels = vertexResult.labels;
    shader->labels.insert(shader->labels.end(), fragmentResult.labels.begin(), fragmentResult.labels.end());

    // Combine and unique-ify included files
    std::sort(shader->includedFiles.begin(), shader->includedFiles.end());
    shader->includedFiles.erase(std::unique(shader->includedFiles.begin(), shader->includedFiles.end()), shader->includedFiles.end());

    // Unique-ify switch flags
    std::sort(shader->switchFlags.begin(), shader->switchFlags.end(), [](const ShaderPreprocessor::SwitchInfo& a, const ShaderPreprocessor::SwitchInfo& b) {
        return a.name < b.name;
    });
    shader->switchFlags.erase(std::unique(shader->switchFlags.begin(), shader->switchFlags.end(), [](const ShaderPreprocessor::SwitchInfo& a, const ShaderPreprocessor::SwitchInfo& b) {
        return a.name == b.name;
    }), shader->switchFlags.end());

    // Apply default switch states if not already set
    for (const auto& sw : shader->switchFlags) {
        if (m_switchStates.find(sw.name) == m_switchStates.end()) {
            m_switchStates[sw.name] = sw.defaultValue;
        }
    }
    
    if (vertexResult.source.empty() || fragmentResult.source.empty() || 
        vertexResult.source.find("#error") != std::string::npos || 
        fragmentResult.source.find("#error") != std::string::npos) {
        shader->lastError = "Failed to preprocess shader files or include error";
        if (m_compilationCallback) {
            m_compilationCallback(name, false, shader->lastError);
        }
        return shader;
    }
    
    std::string errorLog;
    shader->vertexShaderId = compileShader(vertexResult.source, GL_VERTEX_SHADER, errorLog, &vertexResult.lineMappings);
    if (!shader->vertexShaderId) {
        shader->lastError = errorLog;
        if (m_compilationCallback) {
            m_compilationCallback(name, false, shader->lastError);
        }
        return shader;
    }
    
    shader->fragmentShaderId = compileShader(fragmentResult.source, GL_FRAGMENT_SHADER, errorLog, &fragmentResult.lineMappings);
    if (!shader->fragmentShaderId) {
        shader->lastError = errorLog;
        glDeleteShader(shader->vertexShaderId);
        shader->vertexShaderId = 0;
        if (m_compilationCallback) {
            m_compilationCallback(name, false, shader->lastError);
        }
        return shader;
    }
    
    shader->programId = linkProgram(shader->vertexShaderId, shader->fragmentShaderId, errorLog);
    if (!shader->programId) {
        shader->lastError = errorLog;
        glDeleteShader(shader->vertexShaderId);
        glDeleteShader(shader->fragmentShaderId);
        shader->vertexShaderId = 0;
        shader->fragmentShaderId = 0;
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
                nameStr == "iTime" || nameStr == "iResolution" || nameStr == "iMouse" ||
                nameStr == "u_progressive_fill" || nameStr == "u_render_phase" ||
                nameStr == "u_renderChunkFactor" || nameStr == "u_time_offset" || nameStr == "u_chunk_stride") {
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
            
            // Apply ranges from pragma
            auto allRanges = vertexResult.uniformRanges;
            allRanges.insert(allRanges.end(), fragmentResult.uniformRanges.begin(), fragmentResult.uniformRanges.end());
            for (const auto& r : allRanges) {
                if (r.name == nameStr) {
                    uniform.min = r.min;
                    uniform.max = r.max;
                    break;
                }
            }

            // Apply labels from pragma
            for (const auto& l : shader->labels) {
                if (l.name == nameStr) {
                    uniform.label = l.label;
                    break;
                }
            }

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

void ShaderManager::renderToFramebuffer(const std::string& name, int width, int height, float time, float renderScaleFactor, RenderScaleMode scaleMode) {
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

    // Always ensure framebuffer is allocated at full resolution (or larger)
    // to avoid reallocations when switching modes
    int targetAllocWidth = width;
    int targetAllocHeight = height;

    auto it = m_framebuffers.find(name);
    if (it == m_framebuffers.end()) {
        m_framebuffers[name] = std::make_unique<Framebuffer>(targetAllocWidth, targetAllocHeight);
    } else if (it->second->getWidth() != targetAllocWidth || it->second->getHeight() != targetAllocHeight) {
        // Only resize if the full resolution target changes
        it->second->resize(targetAllocWidth, targetAllocHeight);
    }
    
    // UPSCALING LOGIC
    // If we are in Chunk mode, but the current buffer content is scaled down (from a previous Resolution render),
    // we need to upscale the content to fill the buffer before we start rendering sparse chunks.
    if (chunkMode) {
        auto scaleIt = m_framebufferScales.find(name);
        if (scaleIt != m_framebufferScales.end()) {
            float sx = scaleIt->second.first;
            float sy = scaleIt->second.second;
            // Epsilon check for < 1.0
            if (sx < 0.99f || sy < 0.99f) {
                performUpscale(name, targetAllocWidth, targetAllocHeight, sx, sy);
            }
        }
    }
    
    // Calculate valid UV region for this frame
    if (width > 0 && height > 0) {
        m_framebufferScales[name] = {
            static_cast<float>(scaledWidth) / static_cast<float>(targetAllocWidth),
            static_cast<float>(scaledHeight) / static_cast<float>(targetAllocHeight)
        };
    } else {
        m_framebufferScales[name] = {1.0f, 1.0f};
    }

    // Set texture filtering
    // Always use LINEAR filtering for smoother results when scaling
    m_framebuffers[name]->setFilter(GL_LINEAR);

    m_framebuffers[name]->bind();
    glViewport(0, 0, scaledWidth, scaledHeight); // Set viewport to scaled dimensions (renders to bottom-left corner)
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

std::pair<float, float> ShaderManager::getFramebufferUVScale(const std::string& name) {
    auto it = m_framebufferScales.find(name);
    if (it != m_framebufferScales.end()) {
        return it->second;
    }
    return {1.0f, 1.0f};
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

void ShaderManager::setupSimpleTextureProgram() {
    const char* vertexSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";
    
    const char* fragmentSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform sampler2D screenTexture;
        void main() {
            FragColor = texture(screenTexture, TexCoord);
        }
    )";
    
    std::string errorLog;
    GLuint vs = compileShader(vertexSource, GL_VERTEX_SHADER, errorLog);
    if (!vs) LOG_ERROR("Failed to compile internal vertex shader: {}", errorLog);
    
    GLuint fs = compileShader(fragmentSource, GL_FRAGMENT_SHADER, errorLog);
    if (!fs) LOG_ERROR("Failed to compile internal fragment shader: {}", errorLog);
    
    m_simpleTextureProgram.programId = linkProgram(vs, fs, errorLog);
    if (!m_simpleTextureProgram.programId) LOG_ERROR("Failed to link internal shader: {}", errorLog);
    
    m_simpleTextureProgram.textureLocation = glGetUniformLocation(m_simpleTextureProgram.programId, "screenTexture");
    
    if (vs) glDeleteShader(vs);
    if (fs) glDeleteShader(fs);
}

void ShaderManager::performUpscale(const std::string& name, int width, int height, float scaleX, float scaleY) {
    if (m_simpleTextureProgram.programId == 0) return;
    
    auto it = m_framebuffers.find(name);
    if (it == m_framebuffers.end()) return;
    
    int srcW = static_cast<int>(width * scaleX);
    int srcH = static_cast<int>(height * scaleY);
    
    if (srcW <= 0 || srcH <= 0) return;

    // Create temp texture
    GLuint tempTex;
    glGenTextures(1, &tempTex);
    glBindTexture(GL_TEXTURE_2D, tempTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, srcW, srcH, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Copy valid region from FBO to temp texture
    it->second->bind();
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, srcW, srcH);
    
    // Render temp texture to full FBO
    glViewport(0, 0, width, height);
    glUseProgram(m_simpleTextureProgram.programId);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tempTex);
    glUniform1i(m_simpleTextureProgram.textureLocation, 0);
    
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    it->second->unbind();
    
    glDeleteTextures(1, &tempTex);
    
    // Update scale to 1.0 since we filled the buffer
    m_framebufferScales[name] = {1.0f, 1.0f};
    
    LOG_DEBUG("Upscaled framebuffer '{}' from {}x{} to {}x{}", name, srcW, srcH, width, height);
}
