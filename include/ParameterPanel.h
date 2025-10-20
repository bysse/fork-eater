#pragma once

#include "imgui.h"
#include "ShaderManager.h"
#include "ShaderProject.h"
#include <memory>

class ParameterPanel {
public:
    ParameterPanel(std::shared_ptr<ShaderManager> shaderManager, std::shared_ptr<ShaderProject> shaderProject);

    void render(const std::string& shaderName);
    void setProject(std::shared_ptr<ShaderProject> shaderProject);

private:
    std::shared_ptr<ShaderManager> m_shaderManager;
    std::shared_ptr<ShaderProject> m_shaderProject;
};