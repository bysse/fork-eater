#include "ShaderEditor.h"
#include "ShaderManager.h"
#include "FileWatcher.h"
#include "PreviewPanel.h"
#include "MenuSystem.h"
#include "LeftPanel.h"
#include "FileManager.h"
#include "ParameterPanel.h"

#include "Timeline.h"
#include "ShortcutManager.h"
#include "ShaderProject.h"
#include "Logger.h"
#include "Settings.h"
#include "glad.h"
#include <filesystem>
#include <chrono>
#include <iomanip>

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

    , m_timelineHeight(65.0f) 
    , m_exitRequested(false)
    , m_showShortcutsHelp(false)
    , m_reloadProject(false)
    , m_screenWidth(1280)
    , m_screenHeight(720)
    , m_renderScaleFactor(1.0f) {
    
    // Create component classes
    m_previewPanel = std::make_unique<PreviewPanel>(m_shaderManager);
    m_menuSystem = std::make_unique<MenuSystem>();
    m_parameterPanel = std::make_shared<ParameterPanel>(m_shaderManager, m_currentProject);
    m_leftPanel = std::make_unique<LeftPanel>(m_shaderManager, m_parameterPanel);
    m_fileManager = std::make_unique<FileManager>(m_shaderManager, m_fileWatcher);

    m_timeline = std::make_unique<Timeline>();
    m_shortcutManager = std::make_unique<ShortcutManager>();
    m_currentProject = std::make_shared<ShaderProject>();
}

void ShaderEditor::setScreenSize(int width, int height) {
    m_screenWidth = width;
    m_screenHeight = height;
}

void ShaderEditor::render() {
    // Process any pending shader reloads on the main thread
    processPendingReloads();
    processProjectReload();
    
    // Render all passes to their framebuffers
    if (m_currentProject) {
        const auto& passes = m_currentProject->getPasses();
        for (const auto& pass : passes) {
            if (pass.enabled) {
                int width, height;
                auto it = m_passOutputSizes.find(pass.name);
                if (it != m_passOutputSizes.end()) {
                    width = it->second.first;
                    height = it->second.second;
                } else {
                    width = m_screenWidth;
                    height = m_screenHeight;
                }
                auto startTime = glfwGetTime();
                m_shaderManager->renderToFramebuffer(pass.name, width, height, m_timeline->getCurrentTime(), m_renderScaleFactor);
                auto endTime = glfwGetTime();
                auto duration = endTime - startTime;
                float currentFPS = (duration > 1e-6) ? (1.0f / static_cast<float>(duration)) : 0.0f;
                m_timeline->addFPS(m_timeline->getCurrentTime(), currentFPS, m_renderScaleFactor);

                // Adjust render scale factor based on FPS
                Settings& settings = Settings::getInstance();
                if (currentFPS < settings.getLowFPSRenderThreshold25()) {
                    m_renderScaleFactor = 0.25f;
                } else if (currentFPS < settings.getLowFPSRenderThreshold50()) {
                    m_renderScaleFactor = 0.5f;
                } else {
                    m_renderScaleFactor = 1.0f;
                }
            }
        }
    }

    // Render menu bar in main viewport
    if (ImGui::BeginMainMenuBar()) {
        m_menuSystem->renderMenuBar();
        ImGui::EndMainMenuBar();
    }
    
    // Sync settings between components
    m_previewPanel->setAspectMode(m_menuSystem->getAspectMode());
    m_fileManager->setAutoReload(m_menuSystem->isAutoReloadEnabled());

    
    // Get the main viewport and create a fullscreen window
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                                   ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings | 
                                   ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNavInputs;
    
    // Push style to ensure no scrollbars appear anywhere
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 0.0f);
    
    if (ImGui::Begin("MainLayout", nullptr, window_flags)) {
        renderMainLayout();
    }
    ImGui::End();
    
    ImGui::PopStyleVar(); // ScrollbarSize
    
    // Update timeline
    m_timeline->update(ImGui::GetIO().DeltaTime);
    
    // Render settings window if requested
    m_menuSystem->renderSettingsWindow();
    
    // Show shortcuts help if requested
    showShortcutsHelp();
    
    // Ensure ImGui doesn't capture navigation keys for internal use
    ImGuiIO& io = ImGui::GetIO();
    io.WantCaptureKeyboard = false; // Let our shortcuts always work
}

ShaderEditor::~ShaderEditor() {
    // Components will be automatically destroyed
}

bool ShaderEditor::initialize() {
    // Apply scaled initial timeline height so it starts fully visible on HiDPI
    {
        float scale = Settings::getInstance().getUIScaleFactor();
        ImGuiStyle& style = ImGui::GetStyle();
        float outerChrome = style.WindowPadding.y * 2.0f + style.ChildBorderSize * 2.0f; // TimelinePanel child has border+padding
        float scaledInit = Timeline::defaultHeightDIP() * scale + outerChrome;
        if (m_timelineHeight < scaledInit) m_timelineHeight = scaledInit;
    }

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
    
    return true;
}

void ShaderEditor::setupCallbacks() {
    // Menu system callbacks
    m_menuSystem->onExit = [this]() {
        LOG_INFO("File->Exit selected");
        std::exit(0);  // Immediate exit to avoid cleanup hanging
    };
    
    m_menuSystem->onShowHelp = [this]() {
        m_showShortcutsHelp = true;
    };

    m_menuSystem->onTakeScreenshot = [this]() {
        takeScreenshot();
    };

    m_menuSystem->onScreenSizeChanged = [this](int width, int height) {
        setScreenSize(width, height);
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

    m_leftPanel->onPassesChanged = [this]() {
        if (m_currentProject) {
            m_shaderManager->clearShaders();
            m_currentProject->loadShadersIntoManager(m_shaderManager);
        }
    };
    
}



void ShaderEditor::handleResize(int width, int height) {
    // Handle window resize if needed
}

void ShaderEditor::renderMainLayout() {
    ImVec2 windowSize = ImGui::GetContentRegionAvail();

    // Scale-aware minimums
    float uiScale = Settings::getInstance().getUIScaleFactor();
    ImGuiStyle& style = ImGui::GetStyle();
    float outerChrome = style.WindowPadding.y * 2.0f + style.ChildBorderSize * 2.0f; // padding+border of TimelinePanel
    float timelineMin = Timeline::defaultHeightDIP() * uiScale + outerChrome; // content min + chrome
    if (m_timelineHeight < timelineMin) m_timelineHeight = timelineMin;
    
    // Reserve space for timeline at the bottom (others give up space first)
    float availableHeight = windowSize.y - m_timelineHeight - 4; // 4px for splitter
    if (availableHeight < 0.0f) availableHeight = 0.0f;
    
    // Define flags for child windows without navigation
    ImGuiWindowFlags noNavFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | 
                                  ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNavInputs;
    
    // Create main content area (everything except timeline)
    ImGui::BeginChild("MainContent", ImVec2(windowSize.x, availableHeight), false, noNavFlags);
    
    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    
    // Create horizontal splitter between left panel and right side
    if (m_menuSystem->shouldShowLeftPanel()) {
        // Left panel - keep scrollbar but disable navigation
        ImGui::BeginChild("LeftPanel", ImVec2(m_leftPanelWidth, contentSize.y), true, 
                         ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNavInputs);
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
    ImGui::BeginChild("RightSide", contentSize, false, noNavFlags);

    ImVec2 rightContentSize = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("PreviewPanel", rightContentSize, true, noNavFlags);
    GLuint finalTexture = m_shaderManager->getFramebufferTexture(m_selectedShader);
    m_previewPanel->render(finalTexture, m_timeline->getCurrentTime(), m_renderScaleFactor);
    ImGui::EndChild();
    
    ImGui::EndChild(); // End RightSide
    ImGui::EndChild(); // End MainContent
    
    // Horizontal splitter between main content and timeline
    ImGui::Button("##timeline_splitter", ImVec2(windowSize.x, 4));
    if (ImGui::IsItemActive()) {
        m_timelineHeight -= ImGui::GetIO().MouseDelta.y;
        // Clamp to scaled minimum so timeline cannot be collapsed
        float uiScale = Settings::getInstance().getUIScaleFactor();
        ImGuiStyle& style = ImGui::GetStyle();
        float outerChrome = style.WindowPadding.y * 2.0f + style.ChildBorderSize * 2.0f;
        float timelineMin = Timeline::defaultHeightDIP() * uiScale + outerChrome;
        if (m_timelineHeight < timelineMin) m_timelineHeight = timelineMin;
        // Prevent taking more than the window can afford; leave at least a small area for top (can be 0)
        float maxTimeline = windowSize.y - 4.0f; // allow shrinking main to 0 if needed
        if (m_timelineHeight > maxTimeline) m_timelineHeight = maxTimeline;
    }
    
    // Timeline panel at the bottom
    ImGui::BeginChild("TimelinePanel", ImVec2(windowSize.x, m_timelineHeight), true, noNavFlags);
    m_timeline->render(m_renderScaleFactor);
    ImGui::EndChild();
}

void ShaderEditor::onShaderCompiled(const std::string& name, bool success, const std::string& error) {
    std::string logMessage = "[" + name + "] " + (success ? "SUCCESS" : "ERROR") + ": " + error;
    
    // Output to terminal as well using Logger
    if (success) {
        LOG_SUCCESS("{}", logMessage);
    } else {
        LOG_ERROR("{}", logMessage);
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

    m_shortcutManager->registerShortcut(GLFW_KEY_F2, KeyModifier::None,
        [this]() { takeScreenshot(); },
        "F2", "Take Screenshot", "File");
}

void ShaderEditor::openProject(const std::string& projectPath) {
    if (loadProjectFromPath(projectPath)) {
        m_currentProjectPath = projectPath;
        m_parameterPanel->setProject(m_currentProject);
        LOG_SUCCESS("Opened project: {}", m_currentProject->getManifest().name);
    }
}

void ShaderEditor::setupFileWatching() {
    setupProjectFileWatching();
}

bool ShaderEditor::loadProjectFromPath(const std::string& projectPath) {
    // Clear current shaders
    m_shaderManager->clearShaders();
    m_passOutputSizes.clear();
    
    bool success = false;
    
    if (m_currentProject->loadFromDirectory(projectPath)) {
        m_currentProject->loadState(m_shaderManager);
        if (m_currentProject->loadShadersIntoManager(m_shaderManager)) {
            LOG_INFO("Loaded shader project: {}", m_currentProject->getManifest().name);
            m_leftPanel->setCurrentProject(m_currentProject);
            
            // Auto-select the first enabled pass for immediate rendering
            const auto& passes = m_currentProject->getPasses();
            for (const auto& pass : passes) {
                if (pass.width > 0 && pass.height > 0) {
                    m_passOutputSizes[pass.name] = {pass.width, pass.height};
                }
                if (pass.enabled) {
                    m_selectedShader = pass.name;
                    LOG_INFO("Auto-selected shader pass: {}", pass.name);
                    break;
                }
            }
            
            // Auto-start timeline playback
            m_timeline->play();
            LOG_INFO("Started timeline playback automatically");
            
            success = true;
        } else {
            LOG_ERROR("Failed to load shaders from project: {}", projectPath);
        }
    }
    
    return success;
}

void ShaderEditor::setupProjectFileWatching() {
    if (!m_currentProject || !m_fileWatcher) {
        return;
    }

    m_fileWatcher->clearWatches();

    // Watch the manifest file
    std::string manifestPath = m_currentProject->getManifestPath();
    m_fileWatcher->addWatch(manifestPath, 
        [this](const std::string& path) { onManifestFileChanged(path); });
    LOG_DEBUG("Watching manifest file: {}", manifestPath);

    // Watch the lib directory
    std::filesystem::path projectRoot = std::filesystem::path(m_currentProject->getManifestPath()).parent_path();
    std::filesystem::path libDir = projectRoot / "lib";
    if (std::filesystem::exists(libDir)) {
        m_fileWatcher->addWatch(libDir.string(), 
            [this](const std::string& path) { onShaderFileChanged(path); });
        LOG_DEBUG("Watching lib directory: {}", libDir.string());
    }
    
    const auto& passes = m_currentProject->getPasses();
    for (const auto& pass : passes) {
        if (!pass.enabled) continue;

        auto shader = m_shaderManager->getShader(pass.name);
        if (shader) {
            for (const auto& includedFile : shader->includedFiles) {
                m_fileWatcher->addWatch(includedFile, 
                    [this](const std::string& path) { onShaderFileChanged(path); });
                LOG_DEBUG("Watching included file: {}", includedFile);
            }
        }
    }
}

void ShaderEditor::onShaderFileChanged(const std::string& filePath) {
    // Find which shader pass this file belongs to and queue it for reload
    if (!m_currentProject) return;
    
    const auto& passes = m_currentProject->getPasses();
    for (const auto& pass : passes) {
        if (!pass.enabled) continue;
        
        auto shader = m_shaderManager->getShader(pass.name);
        if (shader) {
            for (const auto& includedFile : shader->includedFiles) {
                if (includedFile == filePath) {
                    LOG_DEBUG("File changed, queuing shader reload: {} ({})", pass.name, filePath);
                    
                    // Thread-safely queue the reload
                    {
                        std::lock_guard<std::mutex> lock(m_reloadQueueMutex);
                        m_pendingReloads.push(pass.name);
                    }
                    return; // Found the pass, no need to check further
                }
            }
        }
    }
}

void ShaderEditor::onManifestFileChanged(const std::string& filePath) {
    LOG_INFO("Manifest file changed, queuing project reload: {}", filePath);
    m_reloadProject = true;
}

void ShaderEditor::processProjectReload() {
    if (!m_reloadProject) return;

    m_reloadProject = false;

    LOG_INFO("Reloading project...");

    // Create a new temporary project to load the new manifest
    auto newProject = std::make_shared<ShaderProject>();
    if (newProject->loadFromDirectory(m_currentProjectPath)) {
        // If the new manifest is valid, replace the current project
        m_currentProject = newProject;
        m_shaderManager->clearShaders();
        m_currentProject->loadShadersIntoManager(m_shaderManager);
        m_leftPanel->setCurrentProject(m_currentProject);
        setupProjectFileWatching();
        LOG_SUCCESS("Project reloaded successfully.");
    } else {
        // If the new manifest is invalid, log the errors and keep the old project
        LOG_ERROR("Failed to reload project. Keeping the current version.");
        const auto& errors = newProject->getValidationErrors();
        for (const auto& error : errors) {
            LOG_ERROR("- {}", error);
        }
    }
}

void ShaderEditor::processPendingReloads() {
    std::queue<std::string> reloadQueue;
    {
        std::lock_guard<std::mutex> lock(m_reloadQueueMutex);
        std::swap(reloadQueue, m_pendingReloads);
    }

    bool refreshedWatches = false;

    while (!reloadQueue.empty()) {
        std::string shaderName = reloadQueue.front();
        reloadQueue.pop();
        
        LOG_DEBUG("Processing shader reload: {}", shaderName);
        if (m_shaderManager->reloadShader(shaderName)) {
            auto shader = m_shaderManager->getShader(shaderName);
            if (shader && m_currentProject) {
                m_currentProject->applyUniformsToShader(shaderName, shader);
            }
            refreshedWatches = true;
        }
    }

    if (refreshedWatches) {
        setupProjectFileWatching();
    }
}

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void ShaderEditor::dumpFramebuffer(const std::string& passName, const std::string& outputPath) {
    GLuint textureId = m_shaderManager->getFramebufferTexture(passName);
    if (textureId == 0) {
        LOG_ERROR("Framebuffer for pass '{}' not found.", passName);
        return;
    }

    int width, height;
    glBindTexture(GL_TEXTURE_2D, textureId);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    unsigned char* data = new unsigned char[width * height * 3];
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    stbi_flip_vertically_on_write(1);
    stbi_write_png(outputPath.c_str(), width, height, 3, data, width * 3);

    delete[] data;

    LOG_IMPORTANT("Framebuffer for pass '{}' dumped to {}", passName, outputPath);
}

void ShaderEditor::takeScreenshot() {
    if (m_currentProjectPath.empty() || m_selectedShader.empty()) {
        LOG_ERROR("Cannot take screenshot: No project or shader selected.");
        return;
    }

    std::filesystem::path projectPath(m_currentProjectPath);
    std::filesystem::path screenshotsPath = projectPath / "screenshots";

    if (!std::filesystem::exists(screenshotsPath)) {
        std::filesystem::create_directory(screenshotsPath);
    }

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");
    ss << '_' << std::setfill('0') << std::setw(3) << ms.count();
    std::string filename = "" + ss.str() + ".png";
    std::filesystem::path outputPath = screenshotsPath / filename;

    dumpFramebuffer(m_selectedShader, outputPath.string());
}

void ShaderEditor::openProjectDialog() {
    // For now, just show an info message. 
    // In the future, this could open a native file dialog
    LOG_INFO("Open project dialog requested. Use command line argument to specify project path.");
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
