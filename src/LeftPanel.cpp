#include "LeftPanel.h"
#include "ShaderManager.h"
#include "ShaderProject.h"
#include "ParameterPanel.h"

#include "imgui/imgui.h"
#include <regex>

LeftPanel::LeftPanel(std::shared_ptr<ShaderManager> shaderManager, std::shared_ptr<ParameterPanel> parameterPanel)
    : m_shaderManager(shaderManager), m_parameterPanel(parameterPanel) {
}

void LeftPanel::setCurrentProject(std::shared_ptr<ShaderProject> project) {
    m_currentProject = project;
}

void LeftPanel::render(const std::string& selectedShader) {
    ImVec2 leftSize = ImGui::GetContentRegionAvail();
    
    // Pass list (top half)
    ImGui::BeginChild("PassList", ImVec2(leftSize.x, leftSize.y * 0.4f), true);
    renderPassList();
    ImGui::EndChild();
    
    ImGui::Separator();
    
    // File list (bottom half)
    ImGui::BeginChild("FileList", ImVec2(leftSize.x, leftSize.y * 0.6f - 10), true);
    renderParameters(selectedShader);
    ImGui::EndChild();
}

void LeftPanel::renderPassList() {
    ImGui::Text("Shader Passes");
    ImGui::Separator();
    
    if (m_currentProject && m_currentProject->isLoaded()) {
        auto& passes = m_currentProject->getPasses();
        for (size_t i = 0; i < passes.size(); ++i) {
            auto& pass = passes[i];
            bool selected = (i == 0); // For now, just select the first pass
            std::string displayName = pass.name + (pass.enabled ? "" : " (disabled)");
            if (ImGui::Selectable(displayName.c_str(), selected)) {
                pass.enabled = !pass.enabled;
                if (onPassesChanged) onPassesChanged();
            }
        }
    } else {
        ImGui::Text("No project loaded");
    }
    

}

void LeftPanel::renderParameters(const std::string& selectedShader) {
    m_parameterPanel->render(selectedShader);
}
