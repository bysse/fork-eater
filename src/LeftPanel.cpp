#include "LeftPanel.h"
#include "ShaderManager.h"

#include "imgui/imgui.h"

LeftPanel::LeftPanel(std::shared_ptr<ShaderManager> shaderManager)
    : m_shaderManager(shaderManager) {
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
    
    // For now, just show a single pass
    bool selected = true;
    if (ImGui::Selectable("Main Pass", selected)) {
        // Pass selection logic here
    }
    
    ImGui::Spacing();
    if (ImGui::Button("Add Pass", ImVec2(-1, 0))) {
        // Add new pass logic
    }
}

void LeftPanel::renderFileList(const std::string& selectedShader) {
    ImGui::Text("Shader Files");
    ImGui::Separator();
    
    if (ImGui::Button("New Shader", ImVec2(-1, 0))) {
        if (onNewShader) onNewShader();
    }
    
    ImGui::Spacing();
    
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