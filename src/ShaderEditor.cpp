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
#include <cmath>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

ShaderEditor::ShaderEditor(std::shared_ptr<ShaderManager> shaderManager, 
                           std::shared_ptr<FileWatcher> fileWatcher)
    : m_shaderManager(shaderManager)
    , m_fileWatcher(fileWatcher)
    , m_showLeftPanel(true)
    , m_showErrorPanel(true)
    , m_leftPanelWidth(300.0f)
    , m_errorPanelHeight(200.0f)
    , m_aspectMode(AspectMode::Fixed_16_9)
    , m_autoReload(true)
    , m_exitRequested(false)
    , m_time(0.0f)
    , m_previewVAO(0)
    , m_previewVBO(0)
    , m_previewFramebuffer(0)
    , m_previewTexture(0) {
    
    m_resolution[0] = 1920.0f; // Default 16:9 values, will be updated dynamically
    m_resolution[1] = 1080.0f;
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
    
    // Get the main viewport and create a fullscreen window
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
    
    if (ImGui::Begin("MainLayout", nullptr, window_flags)) {
        renderMainLayout();
    }
    ImGui::End();
    
    // Demo removed
    
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
        ImGui::MenuItem("Left Panel", nullptr, &m_showLeftPanel);
        ImGui::MenuItem("Error Panel", nullptr, &m_showErrorPanel);
        ImGui::EndMenu();
    }
    
    if (ImGui::BeginMenu("Render")) {
        renderRenderMenu();
        ImGui::EndMenu();
    }
    
    if (ImGui::BeginMenu("Options")) {
        ImGui::MenuItem("Auto Reload", nullptr, &m_autoReload);
        ImGui::EndMenu();
    }
}

void ShaderEditor::renderRenderMenu() {
    ImGui::Text("Aspect Ratio:");
    
    const char* aspectModes[] = { "Free", "16:9", "4:3", "1:1 (Square)", "21:9" };
    int currentAspect = static_cast<int>(m_aspectMode);
    if (ImGui::Combo("##aspect", &currentAspect, aspectModes, IM_ARRAYSIZE(aspectModes))) {
        m_aspectMode = static_cast<AspectMode>(currentAspect);
    }
}

ImVec2 ShaderEditor::calculatePreviewSize(ImVec2 availableSize) {
    if (m_aspectMode == AspectMode::Free) {
        return availableSize;
    }
    
    float targetAspect;
    switch (m_aspectMode) {
        case AspectMode::Fixed_16_9:
            targetAspect = 16.0f / 9.0f;
            break;
        case AspectMode::Fixed_4_3:
            targetAspect = 4.0f / 3.0f;
            break;
        case AspectMode::Fixed_1_1:
            targetAspect = 1.0f;
            break;
        case AspectMode::Fixed_21_9:
            targetAspect = 21.0f / 9.0f;
            break;
        default:
            return availableSize;
    }
    
    float availableAspect = availableSize.x / availableSize.y;
    
    ImVec2 previewSize;
    
    if (availableAspect > targetAspect) {
        // Available area is wider than target aspect ratio
        previewSize.y = availableSize.y;
        previewSize.x = previewSize.y * targetAspect;
    } else {
        // Available area is taller than target aspect ratio
        previewSize.x = availableSize.x;
        previewSize.y = previewSize.x / targetAspect;
    }
    
    return previewSize;
}

void ShaderEditor::renderMainLayout() {
    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    
    // Create horizontal splitter between left panel and right side
    if (m_showLeftPanel) {
        // Left panel
        ImGui::BeginChild("LeftPanel", ImVec2(m_leftPanelWidth, windowSize.y), true);
        renderLeftPanel();
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Vertical splitter
        ImGui::Button("##vsplitter", ImVec2(4, windowSize.y));
        if (ImGui::IsItemActive()) {
            m_leftPanelWidth += ImGui::GetIO().MouseDelta.x;
            if (m_leftPanelWidth < 150.0f) m_leftPanelWidth = 150.0f;
            if (m_leftPanelWidth > windowSize.x - 150.0f) m_leftPanelWidth = windowSize.x - 150.0f;
        }
        ImGui::SameLine();
        
        windowSize.x -= m_leftPanelWidth + 4;
    }
    
    // Right side - preview and error panels
    ImGui::BeginChild("RightSide", windowSize, false);
    
    // Always show preview panel
    float previewHeight = m_showErrorPanel ? windowSize.y - m_errorPanelHeight - 4 : windowSize.y;
    ImGui::BeginChild("PreviewPanel", ImVec2(windowSize.x, previewHeight), true);
    renderPreviewPanel();
    ImGui::EndChild();
    
    if (m_showErrorPanel) {
        // Horizontal splitter
        ImGui::Button("##hsplitter", ImVec2(windowSize.x, 4));
        if (ImGui::IsItemActive()) {
            m_errorPanelHeight -= ImGui::GetIO().MouseDelta.y;
            if (m_errorPanelHeight < 100.0f) m_errorPanelHeight = 100.0f;
            if (m_errorPanelHeight > windowSize.y - 100.0f) m_errorPanelHeight = windowSize.y - 100.0f;
        }
    }
    
    if (m_showErrorPanel) {
        float errorHeight = m_errorPanelHeight;
        ImGui::BeginChild("ErrorPanel", ImVec2(windowSize.x, errorHeight), true);
        renderErrorPanel();
        ImGui::EndChild();
    }
    
    ImGui::EndChild();
}

void ShaderEditor::renderLeftPanel() {
    ImVec2 leftSize = ImGui::GetContentRegionAvail();
    
    // Pass list (top half)
    ImGui::BeginChild("PassList", ImVec2(leftSize.x, leftSize.y * 0.4f), true);
    renderPassList();
    ImGui::EndChild();
    
    ImGui::Separator();
    
    // File list (bottom half)
    ImGui::BeginChild("FileList", ImVec2(leftSize.x, leftSize.y * 0.6f - 10), true);
    renderFileList();
    ImGui::EndChild();
}

void ShaderEditor::renderPassList() {
    ImGui::Text("Shader Passes");
    ImGui::Separator();
    
    // For now, just show a single pass
    bool selected = true;
    if (ImGui::Selectable("Main Pass", selected)) {
        // Pass selection logic here
    }
    
    ImGui::Spacing();
    if (ImGui::Button("Add Pass", ImVec2(-1, 0))) {
        // Add new pass logic
    }
}

void ShaderEditor::renderFileList() {
    ImGui::Text("Shader Files");
    ImGui::Separator();
    
    if (ImGui::Button("New Shader", ImVec2(-1, 0))) {
        createNewShader();
    }
    
    ImGui::Spacing();
    
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
        
        // Show file path as tooltip
        if (ImGui::IsItemHovered()) {
            auto shader = m_shaderManager->getShader(name);
            if (shader) {
                ImGui::BeginTooltip();
                ImGui::Text("Vertex: %s", shader->vertexPath.c_str());
                ImGui::Text("Fragment: %s", shader->fragmentPath.c_str());
                ImGui::EndTooltip();
            }
        }
    }
}

// Shader editing is now done externally - this method is no longer needed

void ShaderEditor::renderErrorPanel() {
    ImGui::Text("Compilation Log");
    
    if (ImGui::Button("Clear")) {
        m_compileLog.clear();
    }
    
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_autoReload); // Reuse this flag for auto-scroll
    
    ImGui::Separator();
    
    // Show compilation log in a scrollable text area
    ImVec2 textSize = ImGui::GetContentRegionAvail();
    textSize.y -= 30; // Leave space for buttons above
    
    ImGui::BeginChild("LogArea", textSize, false, ImGuiWindowFlags_HorizontalScrollbar);
    
    if (!m_compileLog.empty()) {
        ImGui::TextUnformatted(m_compileLog.c_str());
        
        // Auto-scroll to bottom if enabled
        if (m_autoReload && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No compilation messages");
    }
    
    ImGui::EndChild();
}

void ShaderEditor::renderPreviewPanel() {
    ImGui::Text("Shader Preview");
    
    // Show current aspect ratio
    const char* aspectNames[] = { "Free", "16:9", "4:3", "1:1", "21:9" };
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(%s)", aspectNames[static_cast<int>(m_aspectMode)]);
    
    ImGui::Separator();
    
    if (!m_selectedShader.empty()) {
        ImGui::Text("Current: %s", m_selectedShader.c_str());
        
        auto shader = m_shaderManager->getShader(m_selectedShader);
        if (shader && shader->isValid) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "✓ Shader compiled successfully");
            
            // Calculate preview size based on aspect ratio settings
            ImVec2 availableSize = ImGui::GetContentRegionAvail();
            availableSize.y -= 60; // Leave space for text above
            
            ImVec2 previewSize = calculatePreviewSize(availableSize);
            
            // Update uniforms with current preview resolution
            m_shaderManager->useShader(m_selectedShader);
            m_shaderManager->setUniform("u_time", m_time);
            m_resolution[0] = previewSize.x;
            m_resolution[1] = previewSize.y;
            m_shaderManager->setUniform("u_resolution", m_resolution, 2);
            
            // Center the preview if it's smaller than available space
            ImVec2 offset = ImVec2(0.0f, 0.0f); //ImVec2((availableSize.x - previewSize.x) * 0.5f, (availableSize.y - previewSize.y) * 0.5f);
            ImVec2 drawOffset = ImVec2(0.0f, 0.0f); // Offset to use for drawing coordinates
            /*
            if (offset.x > 0) {
                float newPosX = ImGui::GetCursorPosX() + offset.x;
                if (newPosX >= 0) {
                    ImGui::SetCursorPosX(newPosX);
                    drawOffset.x = offset.x;
                }
            }
            if (offset.y > 0) {
                float newPosY = ImGui::GetCursorPosY() + offset.y;
                if (newPosY >= 0) {
                    ImGui::SetCursorPosY(newPosY);
                    drawOffset.y = offset.y;
                }
            }
            */
            // Draw preview
            ImGui::Dummy(previewSize);
            
            // Draw a placeholder with proper aspect ratio
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 screen_pos = ImGui::GetCursorScreenPos();
            ImVec2 canvas_pos = ImVec2(screen_pos.x - drawOffset.x, screen_pos.y - previewSize.y - drawOffset.y);
            ImVec2 canvas_size = previewSize;
            
            // Background (dark gray)
            draw_list->AddRectFilled(canvas_pos, 
                                   ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                                   IM_COL32(30, 30, 30, 255));
            
            // Preview border
            draw_list->AddRect(canvas_pos, 
                             ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                             IM_COL32(100, 100, 100, 255));
            
            // Animated content
            float r = 0.5f + 0.5f * sinf(m_time);
            float g = 0.5f + 0.5f * sinf(m_time + 2.0f);
            float b = 0.5f + 0.5f * sinf(m_time + 4.0f);
            
            float margin = 10.0f;
            draw_list->AddRectFilled(ImVec2(canvas_pos.x + margin, canvas_pos.y + margin),
                                   ImVec2(canvas_pos.x + canvas_size.x - margin, canvas_pos.y + canvas_size.y - margin),
                                   IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), 255));
            
            // Resolution text
            char resText[64];
            snprintf(resText, sizeof(resText), "%.0fx%.0f", previewSize.x, previewSize.y);
            draw_list->AddText(ImVec2(canvas_pos.x + 10, canvas_pos.y + 10), 
                             IM_COL32(255, 255, 255, 200), resText);
                             
            // Preview label
            draw_list->AddText(ImVec2(canvas_pos.x + canvas_size.x/2 - 30, canvas_pos.y + canvas_size.y/2 - 10), 
                             IM_COL32(255, 255, 255, 255), "Preview");
        } else if (shader) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "✗ Compilation failed");
        }
    } else {
        ImGui::Text("No shader selected");
        ImGui::Text("Select a shader from the file list to preview");
    }
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
    std::string logMessage = "[" + name + "] " + (success ? "SUCCESS" : "ERROR") + ": " + error + "\n";
    
    // Add to GUI log
    m_compileLog += logMessage;
    
    // Output to terminal as well
    if (success) {
        std::cout << "\033[32m" << logMessage << "\033[0m"; // Green for success
    } else {
        std::cerr << "\033[31m" << logMessage << "\033[0m"; // Red for errors
    }
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
