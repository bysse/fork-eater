#include "MenuSystem.h"
#include "Settings.h"
#include "Logger.h"
#include <cstdlib>

#include "imgui/imgui.h"

MenuSystem::MenuSystem()
    : m_showLeftPanel(true)
    
    , m_autoReload(true)
    , m_showSettingsWindow(false)
    , m_aspectMode(AspectMode::Fixed_16_9) {
}

void MenuSystem::renderMenuBar() {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Take Screenshot", "F2")) {
            if (onTakeScreenshot) onTakeScreenshot();
        }
        if (ImGui::MenuItem("Exit", "ESC")) {
            if (onExit) onExit();
        }
        ImGui::EndMenu();
    }
    
    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Left Panel", nullptr, &m_showLeftPanel);
        ImGui::EndMenu();
    }
    
    if (ImGui::BeginMenu("Render")) {
        renderRenderMenu();
        ImGui::EndMenu();
    }
    
    if (ImGui::BeginMenu("Options")) {
        ImGui::MenuItem("Auto Reload", nullptr, &m_autoReload);
        ImGui::Separator();
        if (ImGui::MenuItem("Settings...")) {
            m_showSettingsWindow = true;
        }
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

    ImGui::Separator();

    ImGui::Text("Screen Size:");
    if (ImGui::MenuItem("1280x720")) {
        if (onScreenSizeChanged) onScreenSizeChanged(1280, 720);
    }
    if (ImGui::MenuItem("1920x1080")) {
        if (onScreenSizeChanged) onScreenSizeChanged(1920, 1080);
    }
}

void MenuSystem::renderSettingsWindow() {
    if (!m_showSettingsWindow) {
        return;
    }
    
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", &m_showSettingsWindow)) {
        Settings& settings = Settings::getInstance();
        
        ImGui::Text("Display Settings");
        ImGui::Separator();
        
        // DPI Scale Mode
        const char* scaleModes[] = { "Auto-detect", "Manual", "Disabled (1.0x)" };
        int currentMode = static_cast<int>(settings.getDPIScaleMode());
        if (ImGui::Combo("DPI Scale Mode", &currentMode, scaleModes, IM_ARRAYSIZE(scaleModes))) {
            settings.setDPIScaleMode(static_cast<DPIScaleMode>(currentMode));
        }
        
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Auto-detect: Automatically detect system DPI scaling");
            ImGui::Text("Manual: Set custom scaling factors");
            ImGui::Text("Disabled: Use 1.0x scaling (no scaling)");
            ImGui::EndTooltip();
        }
        
        // Show current detected scale if auto mode
        if (settings.getDPIScaleMode() == DPIScaleMode::Auto) {
            float detectedScale = settings.detectSystemDPIScale();
            ImGui::Text("Detected scale: %.2fx", detectedScale);
        }
        
        // Manual scaling controls
        if (settings.getDPIScaleMode() == DPIScaleMode::Manual) {
            ImGui::Spacing();
            
            float uiScale = settings.getUIScaleFactor();
            if (ImGui::SliderFloat("UI Scale", &uiScale, 0.5f, 4.0f, "%.2fx")) {
                settings.setUIScaleFactor(uiScale);
            }
            
            float fontScale = settings.getFontScaleFactor();
            if (ImGui::SliderFloat("Font Scale", &fontScale, 0.5f, 4.0f, "%.2fx")) {
                settings.setFontScaleFactor(fontScale);
            }
            
            ImGui::Spacing();
            if (ImGui::Button("Reset to 1.0x")) {
                settings.setUIScaleFactor(1.0f);
                settings.setFontScaleFactor(1.0f);
            }
            ImGui::SameLine();
            if (ImGui::Button("Apply Auto-detected Scale")) {
                float detectedScale = settings.detectSystemDPIScale();
                settings.setUIScaleFactor(detectedScale);
                settings.setFontScaleFactor(detectedScale);
            }
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Current: UI %.2fx, Font %.2fx", 
                    settings.getUIScaleFactor(), settings.getFontScaleFactor());
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Timeline Settings");

        int lowThreshold = static_cast<int>(settings.getLowFPSThreshold());
        if (ImGui::SliderInt("Low FPS Threshold", &lowThreshold, 1, 60)) {
            settings.setLowFPSThreshold(static_cast<float>(lowThreshold));
        }

        int highThreshold = static_cast<int>(settings.getHighFPSThreshold());
        if (ImGui::SliderInt("High FPS Threshold", &highThreshold, 1, 60)) {
            settings.setHighFPSThreshold(static_cast<float>(highThreshold));
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Low FPS Rendering");

        int lowRenderThreshold50 = static_cast<int>(settings.getLowFPSRenderThreshold50());
        if (ImGui::SliderInt("50% Render Threshold", &lowRenderThreshold50, 1, 60)) {
            settings.setLowFPSRenderThreshold50(static_cast<float>(lowRenderThreshold50));
        }

        int lowRenderThreshold25 = static_cast<int>(settings.getLowFPSRenderThreshold25());
        if (ImGui::SliderInt("25% Render Threshold", &lowRenderThreshold25, 1, 60)) {
            settings.setLowFPSRenderThreshold25(static_cast<float>(lowRenderThreshold25));
        }

        ImGui::Spacing();
        if (ImGui::Button("Close")) {
            m_showSettingsWindow = false;
        }
    }
    ImGui::End();
}
