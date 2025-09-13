#pragma once

#include <memory>
#include <string>
#include <functional>

class ShaderManager;

class LeftPanel {
public:
    LeftPanel(std::shared_ptr<ShaderManager> shaderManager);
    ~LeftPanel() = default;
    
    // Render the left panel
    void render(const std::string& selectedShader);
    
    // Callbacks - these will be set by ShaderEditor
    std::function<void(const std::string&)> onShaderSelected;
    std::function<void(const std::string&)> onShaderDoubleClicked;
    std::function<void()> onNewShader;
    
private:
    std::shared_ptr<ShaderManager> m_shaderManager;
    
    // Private methods
    void renderPassList();
    void renderFileList(const std::string& selectedShader);
};