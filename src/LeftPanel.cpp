#include "LeftPanel.h"
#include "ShaderManager.h"
#include "ShaderProject.h"

#include "imgui/imgui.h"

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
    renderFileList(selectedShader);
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

void LeftPanel::renderFileList(const std::string& selectedShader) {
    ImGui::Text("Shader Files");
    ImGui::Separator();
    
    if (m_currentProject && m_currentProject->isLoaded()) {
        const auto& passes = m_currentProject->getPasses();
        
        // Show files from the current project
        for (const auto& pass : passes) {
            if (!pass.enabled) continue;
            
            // Vertex shader file
            if (!pass.vertexShader.empty()) {
                bool isSelected = (pass.name == selectedShader);
                std::string displayName = "📄 " + pass.vertexShader;
                if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                    if (onShaderSelected) onShaderSelected(pass.name);
                }
                
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    if (onShaderDoubleClicked) onShaderDoubleClicked(pass.name);
                }
                
                // Show file path as tooltip
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Vertex shader: %s", pass.vertexShader.c_str());
                    ImGui::Text("Pass: %s", pass.name.c_str());
                    ImGui::EndTooltip();
                }
            }
            
            // Fragment shader file
            if (!pass.fragmentShader.empty()) {
                bool isSelected = (pass.name == selectedShader);
                std::string displayName = "🎨 " + pass.fragmentShader;
                if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                    if (onShaderSelected) onShaderSelected(pass.name);
                }
                
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    if (onShaderDoubleClicked) onShaderDoubleClicked(pass.name);
                }
                
                // Show file path as tooltip
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Fragment shader: %s", pass.fragmentShader.c_str());
                    ImGui::Text("Pass: %s", pass.name.c_str());
                    ImGui::EndTooltip();
                }
            }
        }
    } else {
        // Fallback: show shaders from ShaderManager if no project loaded
        auto shaderNames = m_shaderManager->getShaderNames();
        for (const auto& name : shaderNames) {
            bool isSelected = (name == selectedShader);
            if (ImGui::Selectable(name.c_str(), isSelected)) {
                if (onShaderSelected) onShaderSelected(name);
            }
            
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                if (onShaderDoubleClicked) onShaderDoubleClicked(name);
            }
            
            // Show file path as tooltip
            if (ImGui::IsItemHovered()) {
                auto shader = m_shaderManager->getShader(name);
                if (shader) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Vertex: %s", shader->vertexPath.c_str());
                    ImGui::Text("Fragment: %s", shader->fragmentPath.c_str());
                    ImGui::EndTooltip();
                }
            }
        }
    }
}
