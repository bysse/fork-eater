#include "ErrorPanel.h"

#include "imgui/imgui.h"

ErrorPanel::ErrorPanel()
    : m_autoScroll(true) {
}

void ErrorPanel::render() {
    ImGui::Text("Compilation Log");
    
    if (ImGui::Button("Clear")) {
        clearLog();
    }
    
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);
    
    ImGui::Separator();
    
    // Show compilation log in a scrollable text area
    ImVec2 textSize = ImGui::GetContentRegionAvail();
    textSize.y -= 30; // Leave space for buttons above
    
    ImGui::BeginChild("LogArea", textSize, false, ImGuiWindowFlags_HorizontalScrollbar);
    
    if (!m_compileLog.empty()) {
        ImGui::TextUnformatted(m_compileLog.c_str());
        
        // Auto-scroll to bottom if enabled
        if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No compilation messages");
    }
    
    ImGui::EndChild();
}

void ErrorPanel::addToLog(const std::string& message) {
    m_compileLog += message;
}

void ErrorPanel::clearLog() {
    m_compileLog.clear();
}