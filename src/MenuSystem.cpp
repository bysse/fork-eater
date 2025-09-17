#include "MenuSystem.h"
#include <iostream>
#include <cstdlib>

#include "imgui/imgui.h"

MenuSystem::MenuSystem()
    : m_showLeftPanel(true)
    , m_showErrorPanel(true)
    , m_autoReload(true)
    , m_aspectMode(AspectMode::Fixed_16_9) {
}

void MenuSystem::renderMenuBar() {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Exit", "Alt+F4")) {
            if (onExit) {
                onExit();
            } else {
                std::cout << "File->Exit selected" << std::endl;
                std::exit(0);  // Fallback immediate exit
            }
        }
        ImGui::EndMenu();
    }
    
    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Left Panel", nullptr, &m_showLeftPanel);
        ImGui::MenuItem("Error Panel", nullptr, &m_showErrorPanel);
        ImGui::EndMenu();
    }
    
    if (ImGui::BeginMenu("Render")) {
        renderRenderMenu();
        ImGui::EndMenu();
    }
    
    if (ImGui::BeginMenu("Options")) {
        ImGui::MenuItem("Auto Reload", nullptr, &m_autoReload);
        ImGui::EndMenu();
    }
    
    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("Keyboard Shortcuts", "F1")) {
            if (onShowHelp) onShowHelp();
        }
        ImGui::EndMenu();
    }
}

void MenuSystem::renderRenderMenu() {
    ImGui::Text("Aspect Ratio:");
    
    const char* aspectModes[] = { "Free", "16:9", "4:3", "1:1 (Square)", "21:9" };
    int currentAspect = static_cast<int>(m_aspectMode);
    if (ImGui::Combo("##aspect", &currentAspect, aspectModes, IM_ARRAYSIZE(aspectModes))) {
        m_aspectMode = static_cast<AspectMode>(currentAspect);
    }
}