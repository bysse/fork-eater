#pragma once

#include <functional>
#include "PreviewPanel.h" // For AspectMode enum

class MenuSystem {
public:
    MenuSystem();
    ~MenuSystem() = default;
    
    // Render methods
    void renderMenuBar();
    void renderRenderMenu();
    void renderSettingsWindow();
    
    // State getters/setters
    bool shouldShowLeftPanel() const { return m_showLeftPanel; }
    bool isAutoReloadEnabled() const { return m_autoReload; }
    AspectMode getAspectMode() const { return m_aspectMode; }
    
    void setShowLeftPanel(bool show) { m_showLeftPanel = show; }
    void setAutoReload(bool autoReload) { m_autoReload = autoReload; }
    void setAspectMode(AspectMode mode) { m_aspectMode = mode; }
    
    // Menu action callbacks - these will be set by ShaderEditor
    std::function<void()> onExit;
    std::function<void()> onShowHelp;
    std::function<void(int, int)> onScreenSizeChanged;
    
private:
    // GUI state
    bool m_showLeftPanel;

    bool m_autoReload;
    bool m_showSettingsWindow;
    
    // Render settings
    AspectMode m_aspectMode;
};