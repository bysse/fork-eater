#pragma once

#include <string>
#include <functional>

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
    float getLowFPSTreshold() const { return m_lowFPSTreshold; }
    void setLowFPSTreshold(float treshold);

    float getHighFPSTreshold() const { return m_highFPSTreshold; }
    void setHighFPSTreshold(float treshold);
    
    // Callback for when settings change
    std::function<void()> onSettingsChanged;
    
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
    float m_lowFPSTreshold = 20.0f;
    float m_highFPSTreshold = 30.0f;
    
    // Cache detected DPI scale
    float m_detectedDPIScale = 1.0f;
    bool m_dpiDetected = false;
};