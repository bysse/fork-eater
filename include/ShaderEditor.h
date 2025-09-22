#pragma once

#include <memory>
#include <string>
#include <vector>

// Forward declare ImGui types
struct ImVec2;

class ShaderManager;
class FileWatcher;
class PreviewPanel;
class MenuSystem;
class LeftPanel;
class FileManager;
class ErrorPanel;
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

private:
    std::shared_ptr<ShaderManager> m_shaderManager;
    std::shared_ptr<FileWatcher> m_fileWatcher;
    
    // Component classes
    std::unique_ptr<PreviewPanel> m_previewPanel;
    std::unique_ptr<MenuSystem> m_menuSystem;
    std::unique_ptr<LeftPanel> m_leftPanel;
    std::unique_ptr<FileManager> m_fileManager;
    std::unique_ptr<ErrorPanel> m_errorPanel;
    std::unique_ptr<Timeline> m_timeline;
    std::unique_ptr<ShortcutManager> m_shortcutManager;
    
    // Project management
    std::shared_ptr<ShaderProject> m_currentProject;
    std::string m_currentProjectPath;
    
    // Layout state
    float m_leftPanelWidth;
    float m_errorPanelHeight;
    float m_timelineHeight;
    
    // Editor state
    std::string m_selectedShader;
    bool m_exitRequested;
    bool m_showShortcutsHelp;
    
    // Private methods
    void renderMainLayout();
    void onShaderCompiled(const std::string& name, bool success, const std::string& error);
    
    // Setup callbacks for component classes
    void setupCallbacks();
    void setupShortcuts();
    
public:
    // Project management
    void openProject(const std::string& projectPath);
    void setupFileWatching();
    
private:
    bool loadProjectFromPath(const std::string& projectPath);
    void openProjectDialog();
    void setupProjectFileWatching();
    void onShaderFileChanged(const std::string& filePath);
};