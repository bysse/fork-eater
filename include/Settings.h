#pragma once

#include <string>
#include <functional>
#include "RenderScaleMode.h"

enum class DPIScaleMode {
    Auto,       // Automatically detect DPI scaling
    Manual,     // Use manually set scaling factor
    Disabled    // No scaling (1.0x)
};

class Settings {
public:
    // Get singleton instance
    static Settings& getInstance();
    
    // Initialize settings (load from file)
    void initialize();
    
    // Save settings to file
    void save();
    
    // DPI/UI Scaling settings
    DPIScaleMode getDPIScaleMode() const { return m_dpiScaleMode; }
    void setDPIScaleMode(DPIScaleMode mode);
    
    float getUIScaleFactor() const { return m_uiScaleFactor; }
    void setUIScaleFactor(float factor);
    
    float getFontScaleFactor() const { return m_fontScaleFactor; }
    void setFontScaleFactor(float factor);
    
    bool getAutoDetectDPI() const { return m_dpiScaleMode == DPIScaleMode::Auto; }
    
    // Detect system DPI scaling
    float detectSystemDPIScale();
    
    // Apply current scaling settings to ImGui
    void applyToImGui();
    
    // FPS Thresholds
    float getLowFPSThreshold() const { return m_lowFPSThreshold; }
    void setLowFPSThreshold(float threshold);

    float getHighFPSThreshold() const { return m_highFPSThreshold; }
    void setHighFPSThreshold(float threshold);

    // Low FPS rendering thresholds
    float getLowFPSRenderThreshold50() const { return m_lowFPSRenderThreshold50; }
    void setLowFPSRenderThreshold50(float threshold);

    float getLowFPSRenderThreshold25() const { return m_lowFPSRenderThreshold25; }
    void setLowFPSRenderThreshold25(float threshold);
    
    // Render Scale Mode
    RenderScaleMode getRenderScaleMode() const { return m_renderScaleMode; }
    void setRenderScaleMode(RenderScaleMode mode);

    // Callback for when settings change
    std::function<void()> onSettingsChanged;
    // Callback for when render scale mode changes
    std::function<void()> onRenderScaleModeChanged;
    
private:
    Settings() = default;
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;
    
    // Load/save helpers
    void loadFromFile();
    void saveToFile();
    std::string getSettingsPath() const;
    
    // Settings values
    DPIScaleMode m_dpiScaleMode = DPIScaleMode::Auto;
    float m_uiScaleFactor = 1.0f;
    float m_fontScaleFactor = 1.0f;
    float m_lowFPSThreshold = 20.0f;
    float m_highFPSThreshold = 50.0f;
    float m_lowFPSRenderThreshold50 = 10.0f; // FPS below this will trigger 50% render scale
    float m_lowFPSRenderThreshold25 = 5.0f;  // FPS below this will trigger 25% render scale
    RenderScaleMode m_renderScaleMode = RenderScaleMode::Auto;
    
    // Cache detected DPI scale
    float m_detectedDPIScale = 1.0f;
    bool m_dpiDetected = false;
};