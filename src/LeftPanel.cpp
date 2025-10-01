#include "LeftPanel.h"
#include "ShaderManager.h"
#include "ShaderProject.h"

#include "imgui/imgui.h"
#include <regex>

LeftPanel::LeftPanel(std::shared_ptr<ShaderManager> shaderManager)
    : m_shaderManager(shaderManager) {
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
    renderUniformList(selectedShader);
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

void LeftPanel::renderUniformList(const std::string& selectedShader) {
    ImGui::Text("Shader Uniforms");
    ImGui::Separator();

    if (m_shaderManager && !selectedShader.empty()) {
        std::string source = m_shaderManager->getPreprocessedSource(selectedShader);
        if (!source.empty()) {
            std::regex uniformRegex("uniform\\s+\\w+\\s+([a-zA-Z0-9_]+)");
            std::smatch matches;
            auto it = source.cbegin();
            while (std::regex_search(it, source.cend(), matches, uniformRegex)) {
                if (matches.size() == 2) {
                    std::string uniformName = matches[1].str();
                    bool isSystemUniform = (uniformName == "iTime" || uniformName == "iResolution" || uniformName == "iMouse");
                    if (isSystemUniform) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                    }
                    ImGui::Text("%s", uniformName.c_str());
                    if (isSystemUniform) {
                        ImGui::PopStyleColor();
                    }
                }
                it = matches.suffix().first;
            }
        }
    } else {
        ImGui::Text("No shader selected");
    }
}
