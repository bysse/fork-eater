#pragma once

#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <unordered_map>
#include <deque>

// Forward declare ImGui types
struct ImVec2;

class ShaderManager;
class FileWatcher;
class PreviewPanel;
class MenuSystem;
class LeftPanel;
class FileManager;
class ParameterPanel;

class Timeline;
class ShortcutManager;
class ShaderProject;

class ShaderEditor {
public:
    ShaderEditor(std::shared_ptr<ShaderManager> shaderManager, 
                 std::shared_ptr<FileWatcher> fileWatcher);
    ~ShaderEditor();
    
    // Initialize the editor
    bool initialize();
    
    // Render the editor GUI
    void render();
    
    // Handle window events
    void handleResize(int width, int height);
    
    // Handle keyboard events
    bool handleKeyPress(int key, int scancode, int action, int mods);
    
    // Check if application should exit
    bool shouldExit() const;
    
    // Show shortcuts help window
    void showShortcutsHelp();

    // Get timeline
    Timeline* getTimeline() { return m_timeline.get(); }

    // Get render scale factor
    float getRenderScaleFactor() const { return m_renderScaleFactor; }

    // Set screen size
    void setScreenSize(int width, int height);

private:
    std::shared_ptr<ShaderManager> m_shaderManager;
    std::shared_ptr<FileWatcher> m_fileWatcher;
    
    // Component classes
    std::unique_ptr<PreviewPanel> m_previewPanel;
    std::unique_ptr<MenuSystem> m_menuSystem;
    std::unique_ptr<LeftPanel> m_leftPanel;
    std::unique_ptr<FileManager> m_fileManager;
    std::shared_ptr<ParameterPanel> m_parameterPanel;

    std::unique_ptr<Timeline> m_timeline;
    std::unique_ptr<ShortcutManager> m_shortcutManager;
    
    // Project management
    std::shared_ptr<ShaderProject> m_currentProject;
    std::string m_currentProjectPath;
    
    // Layout state
    float m_leftPanelWidth;

    float m_timelineHeight;
    
    // Editor state
    std::string m_selectedShader;
    bool m_exitRequested;
    bool m_showShortcutsHelp;
    bool m_reloadProject;
    int m_screenWidth;
    int m_screenHeight;
    float m_renderScaleFactor; // Current render scale factor (1.0, 0.5, 0.25)
    std::unordered_map<std::string, std::pair<int, int>> m_passOutputSizes;
    std::deque<float> m_fpsHistory;
    
    // Thread-safe shader reload queue
    std::queue<std::string> m_pendingReloads;
    std::mutex m_reloadQueueMutex;
    
    // Private methods
    void renderMainLayout();
    void onShaderCompiled(const std::string& name, bool success, const std::string& error);
    
    // Setup callbacks for component classes
    void setupCallbacks();
    void setupShortcuts();
    
public:
    // Project management
    void takeScreenshot();
    void openProject(const std::string& projectPath);
    void setupFileWatching();
    void dumpFramebuffer(const std::string& passName, const std::string& outputPath);
    
private:
    bool loadProjectFromPath(const std::string& projectPath);
    void openProjectDialog();
    void setupProjectFileWatching();
    void onShaderFileChanged(const std::string& filePath);
    void onManifestFileChanged(const std::string& filePath);
    void processPendingReloads();
    void processProjectReload();
};