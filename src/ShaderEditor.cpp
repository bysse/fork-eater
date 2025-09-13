#include "ShaderEditor.h"
#include "ShaderManager.h"
#include "FileWatcher.h"
#include "PreviewPanel.h"
#include "MenuSystem.h"
#include "LeftPanel.h"
#include "FileManager.h"
#include "ErrorPanel.h"
#include "Timeline.h"
#include "ShortcutManager.h"

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
    , m_timelineHeight(65.0f)  // Match Timeline::TIMELINE_HEIGHT
    , m_exitRequested(false)
    , m_showShortcutsHelp(false) {
    
    // Create component classes
    m_previewPanel = std::make_unique<PreviewPanel>(m_shaderManager);
    m_menuSystem = std::make_unique<MenuSystem>();
    m_leftPanel = std::make_unique<LeftPanel>(m_shaderManager);
    m_fileManager = std::make_unique<FileManager>(m_shaderManager, m_fileWatcher);
    m_errorPanel = std::make_unique<ErrorPanel>();
    m_timeline = std::make_unique<Timeline>();
    m_shortcutManager = std::make_unique<ShortcutManager>();
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
    
    // Setup keyboard shortcuts
    setupShortcuts();
    
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
    
    m_menuSystem->onShowHelp = [this]() {
        m_showShortcutsHelp = true;
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
    
    // Update timeline
    m_timeline->update(ImGui::GetIO().DeltaTime);
    
    // Show shortcuts help if requested
    showShortcutsHelp();
}

void ShaderEditor::handleResize(int width, int height) {
    // Handle window resize if needed
}

void ShaderEditor::renderMainLayout() {
    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    
    // Reserve space for timeline at the bottom
    float availableHeight = windowSize.y - m_timelineHeight - 4; // 4px for splitter
    
    // Create main content area (everything except timeline)
    ImGui::BeginChild("MainContent", ImVec2(windowSize.x, availableHeight), false);
    
    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    
    // Create horizontal splitter between left panel and right side
    if (m_menuSystem->shouldShowLeftPanel()) {
        // Left panel
        ImGui::BeginChild("LeftPanel", ImVec2(m_leftPanelWidth, contentSize.y), true);
        m_leftPanel->render(m_selectedShader);
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Vertical splitter
        ImGui::Button("##vsplitter", ImVec2(4, contentSize.y));
        if (ImGui::IsItemActive()) {
            m_leftPanelWidth += ImGui::GetIO().MouseDelta.x;
            if (m_leftPanelWidth < 150.0f) m_leftPanelWidth = 150.0f;
            if (m_leftPanelWidth > contentSize.x - 150.0f) m_leftPanelWidth = contentSize.x - 150.0f;
        }
        ImGui::SameLine();
        
        contentSize.x -= m_leftPanelWidth + 4;
    }
    
    // Right side - preview and error panels
    ImGui::BeginChild("RightSide", contentSize, false);
    
    // Always show preview panel
    float previewHeight = m_menuSystem->shouldShowErrorPanel() ? 
                         contentSize.y - m_errorPanelHeight - 4 : contentSize.y;
    ImGui::BeginChild("PreviewPanel", ImVec2(contentSize.x, previewHeight), true);
    m_previewPanel->render(m_selectedShader, m_timeline->getCurrentTime());
    ImGui::EndChild();
    
    if (m_menuSystem->shouldShowErrorPanel()) {
        // Horizontal splitter
        ImGui::Button("##hsplitter", ImVec2(contentSize.x, 4));
        if (ImGui::IsItemActive()) {
            m_errorPanelHeight -= ImGui::GetIO().MouseDelta.y;
            if (m_errorPanelHeight < 100.0f) m_errorPanelHeight = 100.0f;
            if (m_errorPanelHeight > contentSize.y - 100.0f) m_errorPanelHeight = contentSize.y - 100.0f;
        }
    }
    
    if (m_menuSystem->shouldShowErrorPanel()) {
        float errorHeight = m_errorPanelHeight;
        ImGui::BeginChild("ErrorPanel", ImVec2(contentSize.x, errorHeight), true);
        m_errorPanel->render();
        ImGui::EndChild();
    }
    
    ImGui::EndChild(); // End RightSide
    ImGui::EndChild(); // End MainContent
    
    // Horizontal splitter between main content and timeline
    ImGui::Button("##timeline_splitter", ImVec2(windowSize.x, 4));
    if (ImGui::IsItemActive()) {
        m_timelineHeight -= ImGui::GetIO().MouseDelta.y;
        if (m_timelineHeight < 50.0f) m_timelineHeight = 50.0f;  // Reduced minimum
        if (m_timelineHeight > windowSize.y - 200.0f) m_timelineHeight = windowSize.y - 200.0f;
    }
    
    // Timeline panel at the bottom
    ImGui::BeginChild("TimelinePanel", ImVec2(windowSize.x, m_timelineHeight), true);
    m_timeline->render();
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

bool ShaderEditor::handleKeyPress(int key, int scancode, int action, int mods) {
    // Handle shortcuts first
    return m_shortcutManager->handleKeyPress(nullptr, key, scancode, action, mods);
}

void ShaderEditor::setupShortcuts() {
    // Timeline shortcuts
    m_shortcutManager->registerShortcut(GLFW_KEY_SPACE, KeyModifier::None, 
        [this]() { m_timeline->togglePlayPause(); },
        "Space", "Toggle Play/Pause", "Timeline");
    
    // Time navigation - normal steps
    m_shortcutManager->registerShortcut(GLFW_KEY_LEFT, KeyModifier::None, 
        [this]() { m_timeline->jumpTime(-1.0f); },
        "Left Arrow", "Jump back 1 second", "Timeline");
    
    m_shortcutManager->registerShortcut(GLFW_KEY_RIGHT, KeyModifier::None, 
        [this]() { m_timeline->jumpTime(1.0f); },
        "Right Arrow", "Jump forward 1 second", "Timeline");
    
    // Time navigation - fine steps (Ctrl + Arrow)
    m_shortcutManager->registerShortcut(GLFW_KEY_LEFT, KeyModifier::Ctrl, 
        [this]() { m_timeline->jumpTime(-0.1f); },
        "Ctrl + Left Arrow", "Jump back 0.1 second", "Timeline");
    
    m_shortcutManager->registerShortcut(GLFW_KEY_RIGHT, KeyModifier::Ctrl, 
        [this]() { m_timeline->jumpTime(0.1f); },
        "Ctrl + Right Arrow", "Jump forward 0.1 second", "Timeline");
    
    // Time navigation - big steps (Shift + Arrow)
    m_shortcutManager->registerShortcut(GLFW_KEY_LEFT, KeyModifier::Shift, 
        [this]() { m_timeline->jumpTime(-10.0f); },
        "Shift + Left Arrow", "Jump back 10 seconds", "Timeline");
    
    m_shortcutManager->registerShortcut(GLFW_KEY_RIGHT, KeyModifier::Shift, 
        [this]() { m_timeline->jumpTime(10.0f); },
        "Shift + Right Arrow", "Jump forward 10 seconds", "Timeline");
    
    // Home/End navigation
    m_shortcutManager->registerShortcut(GLFW_KEY_HOME, KeyModifier::None, 
        [this]() { m_timeline->jumpToStart(); },
        "Home", "Jump to start", "Timeline");
    
    m_shortcutManager->registerShortcut(GLFW_KEY_END, KeyModifier::None, 
        [this]() { m_timeline->jumpToEnd(); },
        "End", "Jump to end", "Timeline");
    
    // Speed control (Shift + Up/Down)
    m_shortcutManager->registerShortcut(GLFW_KEY_UP, KeyModifier::Shift, 
        [this]() { m_timeline->adjustSpeed(0.1f); },
        "Shift + Up Arrow", "Increase playback speed", "Timeline");
    
    m_shortcutManager->registerShortcut(GLFW_KEY_DOWN, KeyModifier::Shift, 
        [this]() { m_timeline->adjustSpeed(-0.1f); },
        "Shift + Down Arrow", "Decrease playback speed", "Timeline");
    
    // Help shortcut
    m_shortcutManager->registerShortcut(GLFW_KEY_F1, KeyModifier::None, 
        [this]() { m_showShortcutsHelp = true; },
        "F1", "Show keyboard shortcuts", "Help");
}

void ShaderEditor::showShortcutsHelp() {
    if (!m_showShortcutsHelp) return;
    
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Keyboard Shortcuts", &m_showShortcutsHelp)) {
        auto shortcuts = m_shortcutManager->getAllShortcuts();
        
        std::string currentCategory;
        for (const auto& shortcut : shortcuts) {
            if (shortcut.category != currentCategory) {
                if (!currentCategory.empty()) {
                    ImGui::Separator();
                }
                currentCategory = shortcut.category;
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "%s", currentCategory.c_str());
                ImGui::Separator();
            }
            
            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, 200);
            
            ImGui::Text("%s", shortcut.keys.c_str());
            ImGui::NextColumn();
            ImGui::Text("%s", shortcut.description.c_str());
            ImGui::NextColumn();
            
            ImGui::Columns(1);
        }
    }
    ImGui::End();
}
