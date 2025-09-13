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
    
    // State getters/setters
    bool shouldShowLeftPanel() const { return m_showLeftPanel; }
    bool shouldShowErrorPanel() const { return m_showErrorPanel; }
    bool isAutoReloadEnabled() const { return m_autoReload; }
    AspectMode getAspectMode() const { return m_aspectMode; }
    
    void setShowLeftPanel(bool show) { m_showLeftPanel = show; }
    void setShowErrorPanel(bool show) { m_showErrorPanel = show; }
    void setAutoReload(bool autoReload) { m_autoReload = autoReload; }
    void setAspectMode(AspectMode mode) { m_aspectMode = mode; }
    
    // Menu action callbacks - these will be set by ShaderEditor
    std::function<void()> onNewShader;
    std::function<void()> onSave;
    std::function<void()> onExit;
    std::function<void()> onShowHelp;
    
private:
    // GUI state
    bool m_showLeftPanel;
    bool m_showErrorPanel;
    bool m_autoReload;
    
    // Render settings
    AspectMode m_aspectMode;
};