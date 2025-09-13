#include "ShaderEditor.h"
#include "ShaderManager.h"
#include "FileWatcher.h"
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <cstdlib>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

ShaderEditor::ShaderEditor(std::shared_ptr<ShaderManager> shaderManager, 
                           std::shared_ptr<FileWatcher> fileWatcher)
    : m_shaderManager(shaderManager)
    , m_fileWatcher(fileWatcher)
    , m_showDemo(false)
    , m_showShaderList(true)
    , m_showShaderEditor(true)
    , m_showCompileLog(true)
    , m_showPreview(true)
    , m_autoReload(true)
    , m_exitRequested(false)
    , m_time(0.0f)
    , m_previewVAO(0)
    , m_previewVBO(0)
    , m_previewFramebuffer(0)
    , m_previewTexture(0) {
    
    m_resolution[0] = 512.0f;
    m_resolution[1] = 512.0f;
}

ShaderEditor::~ShaderEditor() {
    cleanupPreview();
}

bool ShaderEditor::initialize() {
    // Set up shader compilation callback
    m_shaderManager->setCompilationCallback(
        [this](const std::string& name, bool success, const std::string& error) {
            onShaderCompiled(name, success, error);
        }
    );
    
    // Set up preview quad
    setupPreviewQuad();
    
    // Load default shader if it exists
    m_shaderManager->loadShader("basic", "shaders/basic.vert", "shaders/basic.frag");
    
    return true;
}

void ShaderEditor::render() {
    // Render menu bar in main viewport
    if (ImGui::BeginMainMenuBar()) {
        renderMenuBar();
        ImGui::EndMainMenuBar();
    }
    
    // Render windows as separate windows (no docking for now)
    if (m_showDemo) {
        ImGui::ShowDemoWindow(&m_showDemo);
    }
    
    if (m_showShaderList) {
        renderShaderList();
    }
    
    if (m_showShaderEditor) {
        renderShaderEditor();
    }
    
    if (m_showCompileLog) {
        renderCompileLog();
    }
    
    if (m_showPreview) {
        renderPreview();
    }
    
    // Update time for shader uniforms
    m_time += ImGui::GetIO().DeltaTime;
}

void ShaderEditor::handleResize(int width, int height) {
    // Handle window resize if needed
}

void ShaderEditor::renderMenuBar() {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Shader", "Ctrl+N")) {
            createNewShader();
        }
        if (ImGui::MenuItem("Save", "Ctrl+S")) {
            if (!m_selectedShader.empty()) {
                saveShaderToFile(m_selectedShader);
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Alt+F4")) {
            std::cout << "File->Exit selected" << std::endl;
            std::exit(0);  // Immediate exit to avoid cleanup hanging
        }
        ImGui::EndMenu();
    }
    
    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Shader List", nullptr, &m_showShaderList);
        ImGui::MenuItem("Shader Editor", nullptr, &m_showShaderEditor);
        ImGui::MenuItem("Compile Log", nullptr, &m_showCompileLog);
        ImGui::MenuItem("Preview", nullptr, &m_showPreview);
        ImGui::Separator();
        ImGui::MenuItem("ImGui Demo", nullptr, &m_showDemo);
        ImGui::EndMenu();
    }
    
    if (ImGui::BeginMenu("Options")) {
        ImGui::MenuItem("Auto Reload", nullptr, &m_autoReload);
        ImGui::EndMenu();
    }
}

void ShaderEditor::renderShaderList() {
    ImGui::Begin("Shader List", &m_showShaderList);
    
    if (ImGui::Button("New Shader")) {
        createNewShader();
    }
    
    ImGui::Separator();
    
    auto shaderNames = m_shaderManager->getShaderNames();
    for (const auto& name : shaderNames) {
        bool selected = (name == m_selectedShader);
        if (ImGui::Selectable(name.c_str(), selected)) {
            m_selectedShader = name;
            loadShaderFromFile(name);
        }
        
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            // Double-click to reload
            m_shaderManager->reloadShader(name);
        }
    }
    
    ImGui::End();
}

void ShaderEditor::renderShaderEditor() {
    ImGui::Begin("Shader Editor", &m_showShaderEditor);
    
    if (!m_selectedShader.empty()) {
        ImGui::Text("Editing: %s", m_selectedShader.c_str());
        
        if (ImGui::Button("Compile")) {
            saveShaderToFile(m_selectedShader);
            m_shaderManager->reloadShader(m_selectedShader);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            saveShaderToFile(m_selectedShader);
        }
        
        ImGui::Separator();
        
        // Vertex shader editor
        ImGui::Text("Vertex Shader:");
        char* vertexBuffer = const_cast<char*>(m_vertexShaderText.c_str());
        if (ImGui::InputTextMultiline("##vertex", vertexBuffer, m_vertexShaderText.capacity() + 1, 
                                      ImVec2(-1, 200), ImGuiInputTextFlags_CallbackResize,
                                      [](ImGuiInputTextCallbackData* data) {
                                          if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                                              std::string* str = (std::string*)data->UserData;
                                              str->resize(data->BufTextLen);
                                              data->Buf = const_cast<char*>(str->c_str());
                                          }
                                          return 0;
                                      }, &m_vertexShaderText)) {
            m_vertexShaderText = vertexBuffer;
        }
        
        ImGui::Separator();
        
        // Fragment shader editor
        ImGui::Text("Fragment Shader:");
        char* fragmentBuffer = const_cast<char*>(m_fragmentShaderText.c_str());
        if (ImGui::InputTextMultiline("##fragment", fragmentBuffer, m_fragmentShaderText.capacity() + 1, 
                                      ImVec2(-1, 200), ImGuiInputTextFlags_CallbackResize,
                                      [](ImGuiInputTextCallbackData* data) {
                                          if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                                              std::string* str = (std::string*)data->UserData;
                                              str->resize(data->BufTextLen);
                                              data->Buf = const_cast<char*>(str->c_str());
                                          }
                                          return 0;
                                      }, &m_fragmentShaderText)) {
            m_fragmentShaderText = fragmentBuffer;
        }
    } else {
        ImGui::Text("No shader selected");
        if (ImGui::Button("Create New Shader")) {
            createNewShader();
        }
    }
    
    ImGui::End();
}

void ShaderEditor::renderCompileLog() {
    ImGui::Begin("Compile Log", &m_showCompileLog);
    
    ImGui::TextWrapped("%s", m_compileLog.c_str());
    
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::End();
}

void ShaderEditor::renderPreview() {
    ImGui::Begin("Preview", &m_showPreview);
    
    if (!m_selectedShader.empty()) {
        auto shader = m_shaderManager->getShader(m_selectedShader);
        if (shader && shader->isValid) {
            // Update uniforms
            m_shaderManager->useShader(m_selectedShader);
            m_shaderManager->setUniform("u_time", m_time);
            m_shaderManager->setUniform("u_resolution", m_resolution, 2);
            
            // Render preview quad
            glBindVertexArray(m_previewVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }
    }
    
    ImGui::End();
}

void ShaderEditor::setupPreviewQuad() {
    // Full-screen quad vertices
    float vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
        
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &m_previewVAO);
    glGenBuffers(1, &m_previewVBO);
    
    glBindVertexArray(m_previewVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_previewVBO);
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

void ShaderEditor::cleanupPreview() {
    if (m_previewVAO != 0) {
        glDeleteVertexArrays(1, &m_previewVAO);
        m_previewVAO = 0;
    }
    
    if (m_previewVBO != 0) {
        glDeleteBuffers(1, &m_previewVBO);
        m_previewVBO = 0;
    }
    
    if (m_previewFramebuffer != 0) {
        glDeleteFramebuffers(1, &m_previewFramebuffer);
        m_previewFramebuffer = 0;
    }
    
    if (m_previewTexture != 0) {
        glDeleteTextures(1, &m_previewTexture);
        m_previewTexture = 0;
    }
}

void ShaderEditor::onShaderCompiled(const std::string& name, bool success, const std::string& error) {
    m_compileLog += "[" + name + "] " + (success ? "SUCCESS" : "ERROR") + ": " + error + "\n";
}

void ShaderEditor::onFileChanged(const std::string& filePath) {
    if (m_autoReload) {
        // Find shader that uses this file
        auto shaderNames = m_shaderManager->getShaderNames();
        for (const auto& name : shaderNames) {
            auto shader = m_shaderManager->getShader(name);
            if (shader && (shader->vertexPath == filePath || shader->fragmentPath == filePath)) {
                m_shaderManager->reloadShader(name);
                break;
            }
        }
    }
}

void ShaderEditor::loadShaderFromFile(const std::string& shaderName) {
    auto shader = m_shaderManager->getShader(shaderName);
    if (!shader) return;
    
    std::ifstream vertFile(shader->vertexPath);
    if (vertFile.is_open()) {
        m_vertexShaderText.assign((std::istreambuf_iterator<char>(vertFile)),
                                  std::istreambuf_iterator<char>());
        vertFile.close();
    }
    
    std::ifstream fragFile(shader->fragmentPath);
    if (fragFile.is_open()) {
        m_fragmentShaderText.assign((std::istreambuf_iterator<char>(fragFile)),
                                   std::istreambuf_iterator<char>());
        fragFile.close();
    }
    
    // Set up file watching
    if (m_fileWatcher) {
        m_fileWatcher->addWatch(shader->vertexPath, 
            [this](const std::string& path) { onFileChanged(path); });
        m_fileWatcher->addWatch(shader->fragmentPath, 
            [this](const std::string& path) { onFileChanged(path); });
    }
}

void ShaderEditor::saveShaderToFile(const std::string& shaderName) {
    auto shader = m_shaderManager->getShader(shaderName);
    if (!shader) return;
    
    std::ofstream vertFile(shader->vertexPath);
    if (vertFile.is_open()) {
        vertFile << m_vertexShaderText;
        vertFile.close();
    }
    
    std::ofstream fragFile(shader->fragmentPath);
    if (fragFile.is_open()) {
        fragFile << m_fragmentShaderText;
        fragFile.close();
    }
}

void ShaderEditor::createNewShader() {
    // TODO: Implement new shader creation dialog
    static int counter = 1;
    std::string name = "shader_" + std::to_string(counter++);
    std::string vertPath = "shaders/" + name + ".vert";
    std::string fragPath = "shaders/" + name + ".frag";
    
    // Create default shader content
    m_vertexShaderText = "#version 330 core\nlayout (location = 0) in vec2 aPos;\nlayout (location = 1) in vec2 aTexCoord;\n\nout vec2 TexCoord;\n\nvoid main()\n{\n    gl_Position = vec4(aPos, 0.0, 1.0);\n    TexCoord = aTexCoord;\n}\n";
    m_fragmentShaderText = "#version 330 core\nout vec4 FragColor;\n\nin vec2 TexCoord;\nuniform float u_time;\nuniform vec2 u_resolution;\n\nvoid main()\n{\n    vec2 uv = TexCoord;\n    vec3 col = 0.5 + 0.5 * cos(u_time + uv.xyx + vec3(0, 2, 4));\n    FragColor = vec4(col, 1.0);\n}\n";
    
    // Save to files
    std::ofstream vertFile(vertPath);
    if (vertFile.is_open()) {
        vertFile << m_vertexShaderText;
        vertFile.close();
    }
    
    std::ofstream fragFile(fragPath);
    if (fragFile.is_open()) {
        fragFile << m_fragmentShaderText;
        fragFile.close();
    }
    
    // Load shader
    m_shaderManager->loadShader(name, vertPath, fragPath);
    m_selectedShader = name;
}

bool ShaderEditor::shouldExit() const {
    return m_exitRequested;
}
