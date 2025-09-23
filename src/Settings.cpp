#include "Settings.h"
#include "Logger.h"
#include <GLFW/glfw3.h>
#include <fstream>
#include <filesystem>
#include <cmath>

#include "imgui/imgui.h"

// Simple JSON-like settings parsing (avoiding external dependencies)
#include <sstream>
#include <map>

Settings& Settings::getInstance() {
    static Settings instance;
    return instance;
}

void Settings::initialize() {
    loadFromFile();
    
    // Auto-detect DPI if in auto mode
    if (m_dpiScaleMode == DPIScaleMode::Auto) {
        float detectedScale = detectSystemDPIScale();
        if (detectedScale > 0.0f) {
            m_uiScaleFactor = detectedScale;
            m_fontScaleFactor = detectedScale;
        }
    }
    
    LOG_INFO("Settings initialized - UI Scale: {:.2f}, Font Scale: {:.2f}, Mode: {}", 
             m_uiScaleFactor, m_fontScaleFactor, 
             (m_dpiScaleMode == DPIScaleMode::Auto ? "Auto" : 
              m_dpiScaleMode == DPIScaleMode::Manual ? "Manual" : "Disabled"));
}

void Settings::save() {
    saveToFile();
}

void Settings::setDPIScaleMode(DPIScaleMode mode) {
    if (m_dpiScaleMode != mode) {
        m_dpiScaleMode = mode;
        
        // Apply mode change
        if (mode == DPIScaleMode::Auto) {
            float detectedScale = detectSystemDPIScale();
            if (detectedScale > 0.0f) {
                m_uiScaleFactor = detectedScale;
                m_fontScaleFactor = detectedScale;
            }
        } else if (mode == DPIScaleMode::Disabled) {
            m_uiScaleFactor = 1.0f;
            m_fontScaleFactor = 1.0f;
        }
        
        save();
        if (onSettingsChanged) onSettingsChanged();
    }
}

void Settings::setUIScaleFactor(float factor) {
    // Clamp to reasonable bounds
    factor = std::max(0.5f, std::min(4.0f, factor));
    
    if (std::abs(m_uiScaleFactor - factor) > 0.01f) {
        m_uiScaleFactor = factor;
        if (m_dpiScaleMode == DPIScaleMode::Manual) {
            save();
        }
        if (onSettingsChanged) onSettingsChanged();
    }
}

void Settings::setFontScaleFactor(float factor) {
    // Clamp to reasonable bounds
    factor = std::max(0.5f, std::min(4.0f, factor));
    
    if (std::abs(m_fontScaleFactor - factor) > 0.01f) {
        m_fontScaleFactor = factor;
        if (m_dpiScaleMode == DPIScaleMode::Manual) {
            save();
        }
        if (onSettingsChanged) onSettingsChanged();
    }
}

float Settings::detectSystemDPIScale() {
    if (m_dpiDetected) {
        return m_detectedDPIScale;
    }
    
    // Initialize GLFW if not already done (for DPI detection)
    bool needGLFWInit = !glfwGetCurrentContext();
    if (needGLFWInit && !glfwInit()) {
        LOG_WARN("Failed to initialize GLFW for DPI detection");
        return 1.0f;
    }
    
    float dpiScale = 1.0f;
    
    // Get primary monitor
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor) {
        // Method 1: Use GLFW content scale (preferred on supported platforms)
        float xscale, yscale;
        glfwGetMonitorContentScale(monitor, &xscale, &yscale);
        
        if (xscale > 0.0f && yscale > 0.0f) {
            dpiScale = std::max(xscale, yscale);
            LOG_INFO("Detected DPI scale using content scale: {:.2f}", dpiScale);
        } else {
            // Method 2: Calculate from monitor physical size and resolution
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            int widthMM, heightMM;
            glfwGetMonitorPhysicalSize(monitor, &widthMM, &heightMM);
            
            if (mode && widthMM > 0 && heightMM > 0) {
                // Calculate DPI (dots per inch)
                float dpiX = (float)mode->width / ((float)widthMM / 25.4f);
                float dpiY = (float)mode->height / ((float)heightMM / 25.4f);
                float dpi = std::max(dpiX, dpiY);
                
                // Standard DPI is 96, so scale factor is dpi/96
                dpiScale = dpi / 96.0f;
                
                // Clamp to reasonable values and round to common scaling factors
                if (dpiScale >= 2.8f) dpiScale = 3.0f;
                else if (dpiScale >= 2.3f) dpiScale = 2.5f;
                else if (dpiScale >= 1.8f) dpiScale = 2.0f;
                else if (dpiScale >= 1.3f) dpiScale = 1.5f;
                else if (dpiScale >= 1.15f) dpiScale = 1.25f;
                else dpiScale = 1.0f;
                
                LOG_INFO("Detected DPI scale using physical size: {:.2f} (DPI: {:.1f})", dpiScale, dpi);
            }
        }
    }
    
    if (needGLFWInit) {
        glfwTerminate();
    }
    
    // Cache the result
    m_detectedDPIScale = dpiScale;
    m_dpiDetected = true;
    
    return dpiScale;
}

void Settings::applyToImGui() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Apply font scaling
    if (std::abs(io.FontGlobalScale - m_fontScaleFactor) > 0.01f) {
        io.FontGlobalScale = m_fontScaleFactor;
        LOG_INFO("Applied font scale: {:.2f}", m_fontScaleFactor);
    }
    
    // Apply UI scaling by modifying the style
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Store original values if not already stored
    static bool originalValuesStored = false;
    static float originalFramePadding = 4.0f;
    static float originalItemSpacing = 8.0f;
    static float originalIndentSpacing = 21.0f;
    static float originalScrollbarSize = 14.0f;
    static float originalFrameBorderSize = 0.0f;
    
    if (!originalValuesStored) {
        originalFramePadding = style.FramePadding.x;
        originalItemSpacing = style.ItemSpacing.x;
        originalIndentSpacing = style.IndentSpacing;
        originalScrollbarSize = style.ScrollbarSize;
        originalFrameBorderSize = style.FrameBorderSize;
        originalValuesStored = true;
    }
    
    // Apply scaling to various UI elements
    style.FramePadding.x = originalFramePadding * m_uiScaleFactor;
    style.FramePadding.y = originalFramePadding * m_uiScaleFactor;
    style.ItemSpacing.x = originalItemSpacing * m_uiScaleFactor;
    style.ItemSpacing.y = originalItemSpacing * m_uiScaleFactor;
    style.IndentSpacing = originalIndentSpacing * m_uiScaleFactor;
    style.ScrollbarSize = originalScrollbarSize * m_uiScaleFactor;
    style.FrameBorderSize = originalFrameBorderSize * m_uiScaleFactor;
    
    LOG_INFO("Applied UI scale: {:.2f}", m_uiScaleFactor);
}

void Settings::loadFromFile() {
    std::string settingsPath = getSettingsPath();
    
    if (!std::filesystem::exists(settingsPath)) {
        LOG_INFO("Settings file not found, using defaults");
        return;
    }
    
    try {
        std::ifstream file(settingsPath);
        if (!file.is_open()) {
            LOG_WARN("Failed to open settings file: {}", settingsPath);
            return;
        }
        
        std::string line;
        std::map<std::string, std::string> settings;
        
        // Simple key=value parsing
        while (std::getline(file, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') continue;
            
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                // Trim whitespace
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                
                settings[key] = value;
            }
        }
        
        // Parse settings
        if (settings.count("dpi_scale_mode")) {
            std::string mode = settings["dpi_scale_mode"];
            if (mode == "auto") m_dpiScaleMode = DPIScaleMode::Auto;
            else if (mode == "manual") m_dpiScaleMode = DPIScaleMode::Manual;
            else if (mode == "disabled") m_dpiScaleMode = DPIScaleMode::Disabled;
        }
        
        if (settings.count("ui_scale_factor")) {
            m_uiScaleFactor = std::stof(settings["ui_scale_factor"]);
        }
        
        if (settings.count("font_scale_factor")) {
            m_fontScaleFactor = std::stof(settings["font_scale_factor"]);
        }
        
        LOG_INFO("Loaded settings from: {}", settingsPath);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error loading settings: {}", e.what());
    }
}

void Settings::saveToFile() {
    std::string settingsPath = getSettingsPath();
    
    try {
        // Create directory if it doesn't exist
        std::filesystem::path dir = std::filesystem::path(settingsPath).parent_path();
        if (!std::filesystem::exists(dir)) {
            std::filesystem::create_directories(dir);
        }
        
        std::ofstream file(settingsPath);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open settings file for writing: {}", settingsPath);
            return;
        }
        
        // Write settings in simple key=value format
        file << "# Fork Eater Settings\n";
        file << "# DPI scale mode: auto, manual, disabled\n";
        
        std::string modeStr;
        switch (m_dpiScaleMode) {
            case DPIScaleMode::Auto: modeStr = "auto"; break;
            case DPIScaleMode::Manual: modeStr = "manual"; break;
            case DPIScaleMode::Disabled: modeStr = "disabled"; break;
        }
        
        file << "dpi_scale_mode=" << modeStr << "\n";
        file << "ui_scale_factor=" << m_uiScaleFactor << "\n";
        file << "font_scale_factor=" << m_fontScaleFactor << "\n";
        
        LOG_INFO("Saved settings to: {}", settingsPath);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error saving settings: {}", e.what());
    }
}

std::string Settings::getSettingsPath() const {
    // Store settings in user config directory
    std::string configDir;
    
    const char* xdgConfig = getenv("XDG_CONFIG_HOME");
    if (xdgConfig) {
        configDir = xdgConfig;
    } else {
        const char* home = getenv("HOME");
        if (home) {
            configDir = std::string(home) + "/.config";
        } else {
            configDir = "."; // fallback to current directory
        }
    }
    
    return configDir + "/fork-eater/settings.conf";
}