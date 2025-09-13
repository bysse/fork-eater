#include "ShaderEditor.h"
#include "ShaderManager.h"
#include "FileWatcher.h"
#include "PreviewPanel.h"
#include "MenuSystem.h"
#include "LeftPanel.h"
#include "FileManager.h"
#include "ErrorPanel.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdlib>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

ShaderEditor::ShaderEditor(std::shared_ptr<ShaderManager> shaderManager, 
                           std::shared_ptr<FileWatcher> fileWatcher)
    : m_shaderManager(shaderManager)
    , m_fileWatcher(fileWatcher)
    , m_leftPanelWidth(300.0f)
    , m_errorPanelHeight(200.0f)
    , m_exitRequested(false)
    , m_time(0.0f) {
    
    // Create component classes
    m_previewPanel = std::make_unique<PreviewPanel>(m_shaderManager);
    m_menuSystem = std::make_unique<MenuSystem>();
    m_leftPanel = std::make_unique<LeftPanel>(m_shaderManager);
    m_fileManager = std::make_unique<FileManager>(m_shaderManager, m_fileWatcher);
    m_errorPanel = std::make_unique<ErrorPanel>();
}

ShaderEditor::~ShaderEditor() {
    // Components will be automatically destroyed
}

bool ShaderEditor::initialize() {
    // Set up shader compilation callback
    m_shaderManager->setCompilationCallback(
        [this](const std::string& name, bool success, const std::string& error) {
            onShaderCompiled(name, success, error);
        }
    );
    
    // Initialize components
    if (!m_previewPanel->initialize()) {
        return false;
    }
    
    // Setup callbacks between components
    setupCallbacks();
    
    // Load default shader if it exists
    m_shaderManager->loadShader("basic", "shaders/basic.vert", "shaders/basic.frag");
    
    return true;
}

void ShaderEditor::setupCallbacks() {
    // Menu system callbacks
    m_menuSystem->onNewShader = [this]() {
        m_fileManager->createNewShader();
    };
    
    m_menuSystem->onSave = [this]() {
        if (!m_selectedShader.empty()) {
            m_fileManager->saveShaderToFile(m_selectedShader);
        }
    };
    
    m_menuSystem->onExit = [this]() {
        std::cout << "File->Exit selected" << std::endl;
        std::exit(0);  // Immediate exit to avoid cleanup hanging
    };
    
    // Left panel callbacks
    m_leftPanel->onShaderSelected = [this](const std::string& name) {
        m_selectedShader = name;
        m_fileManager->loadShaderFromFile(name);
    };
    
    m_leftPanel->onShaderDoubleClicked = [this](const std::string& name) {
        // Double-click to reload
        m_shaderManager->reloadShader(name);
    };
    
    m_leftPanel->onNewShader = [this]() {
        m_fileManager->createNewShader();
    };
}

void ShaderEditor::render() {
    // Render menu bar in main viewport
    if (ImGui::BeginMainMenuBar()) {
        m_menuSystem->renderMenuBar();
        ImGui::EndMainMenuBar();
    }
    
    // Sync settings between components
    m_previewPanel->setAspectMode(m_menuSystem->getAspectMode());
    m_fileManager->setAutoReload(m_menuSystem->isAutoReloadEnabled());
    m_errorPanel->setAutoScroll(m_menuSystem->isAutoReloadEnabled()); // Reuse this flag
    
    // Get the main viewport and create a fullscreen window
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                                   ImGuiWindowFlags_NoSavedSettings;
    
    if (ImGui::Begin("MainLayout", nullptr, window_flags)) {
        renderMainLayout();
    }
    ImGui::End();
    
    // Update time for shader uniforms
    m_time += ImGui::GetIO().DeltaTime;
}

void ShaderEditor::handleResize(int width, int height) {
    // Handle window resize if needed
}

void ShaderEditor::renderMainLayout() {
    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    
    // Create horizontal splitter between left panel and right side
    if (m_menuSystem->shouldShowLeftPanel()) {
        // Left panel
        ImGui::BeginChild("LeftPanel", ImVec2(m_leftPanelWidth, windowSize.y), true);
        m_leftPanel->render(m_selectedShader);
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
    float previewHeight = m_menuSystem->shouldShowErrorPanel() ? 
                         windowSize.y - m_errorPanelHeight - 4 : windowSize.y;
    ImGui::BeginChild("PreviewPanel", ImVec2(windowSize.x, previewHeight), true);
    m_previewPanel->render(m_selectedShader, m_time);
    ImGui::EndChild();
    
    if (m_menuSystem->shouldShowErrorPanel()) {
        // Horizontal splitter
        ImGui::Button("##hsplitter", ImVec2(windowSize.x, 4));
        if (ImGui::IsItemActive()) {
            m_errorPanelHeight -= ImGui::GetIO().MouseDelta.y;
            if (m_errorPanelHeight < 100.0f) m_errorPanelHeight = 100.0f;
            if (m_errorPanelHeight > windowSize.y - 100.0f) m_errorPanelHeight = windowSize.y - 100.0f;
        }
    }
    
    if (m_menuSystem->shouldShowErrorPanel()) {
        float errorHeight = m_errorPanelHeight;
        ImGui::BeginChild("ErrorPanel", ImVec2(windowSize.x, errorHeight), true);
        m_errorPanel->render();
        ImGui::EndChild();
    }
    
    ImGui::EndChild();
}

void ShaderEditor::onShaderCompiled(const std::string& name, bool success, const std::string& error) {
    std::string logMessage = "[" + name + "] " + (success ? "SUCCESS" : "ERROR") + ": " + error + "\n";
    
    // Add to GUI log
    m_errorPanel->addToLog(logMessage);
    
    // Output to terminal as well
    if (success) {
        std::cout << "\033[32m" << logMessage << "\033[0m"; // Green for success
    } else {
        std::cerr << "\033[31m" << logMessage << "\033[0m"; // Red for errors
    }
}

bool ShaderEditor::shouldExit() const {
    return m_exitRequested;
}