#pragma once

#include <string>

class ErrorPanel {
public:
    ErrorPanel();
    ~ErrorPanel() = default;
    
    // Render the error panel
    void render();
    
    // Log management
    void addToLog(const std::string& message);
    void clearLog();
    
    // Settings
    void setAutoScroll(bool autoScroll) { m_autoScroll = autoScroll; }
    bool isAutoScrollEnabled() const { return m_autoScroll; }
    
private:
    std::string m_compileLog;
    bool m_autoScroll;
};