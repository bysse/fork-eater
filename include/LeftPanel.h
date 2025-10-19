#pragma once

#include <memory>
#include <string>
#include <functional>

class ShaderManager;
class ShaderProject;
class ParameterPanel;

class LeftPanel {
public:
    LeftPanel(std::shared_ptr<ShaderManager> shaderManager, std::shared_ptr<ParameterPanel> parameterPanel);
    ~LeftPanel() = default;
    
    // Render the left panel
    void render(const std::string& selectedShader);
    
    // Set current project
    void setCurrentProject(std::shared_ptr<ShaderProject> project);
    
    // Callbacks - these will be set by ShaderEditor
    std::function<void(const std::string&)> onShaderSelected;
    std::function<void(const std::string&)> onShaderDoubleClicked;
    std::function<void()> onNewShader;
    std::function<void()> onPassesChanged;
    
private:
    std::shared_ptr<ShaderManager> m_shaderManager;
    std::shared_ptr<ShaderProject> m_currentProject;
    std::shared_ptr<ParameterPanel> m_parameterPanel;
    
    // Private methods
    void renderPassList();
    void renderParameters(const std::string& selectedShader);
};