#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <memory>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    IMPORTANT = 2,
    WARN = 3,
    ERROR = 4,
    OK = 5
};

class Logger {
public:
    // Get singleton instance
    static Logger& getInstance();
    
    // Initialize logger with debug mode setting
    void initialize(bool debugMode = false);
    
    // Set current log level threshold
    void setLogLevel(LogLevel level);
    
    // Get current log level threshold
    LogLevel getLogLevel() const;
    
    // Check if debug mode is enabled
    bool isDebugEnabled() const;
    
    // Main logging functions
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void success(const std::string& message); // For OK/success messages
    void important(const std::string& message);
    
    // Template functions for easy formatting
    template<typename... Args>
    void debug(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::DEBUG)) {
            std::string message = formatMessage(format, std::forward<Args>(args)...);
            logMessage(LogLevel::DEBUG, message);
        }
    }
    
    template<typename... Args>
    void info(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::INFO)) {
            std::string message = formatMessage(format, std::forward<Args>(args)...);
            logMessage(LogLevel::INFO, message);
        }
    }

    template<typename... Args>
    void important(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::IMPORTANT)) {
            std::string message = formatMessage(format, std::forward<Args>(args)...);
            logMessage(LogLevel::IMPORTANT, message);
        }
    }
    
    template<typename... Args>
    void warn(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::WARN)) {
            std::string message = formatMessage(format, std::forward<Args>(args)...);
            logMessage(LogLevel::WARN, message);
        }
    }
    
    template<typename... Args>
    void error(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::ERROR)) {
            std::string message = formatMessage(format, std::forward<Args>(args)...);
            logMessage(LogLevel::ERROR, message);
        }
    }
    
    template<typename... Args>
    void success(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::OK)) {
            std::string message = formatMessage(format, std::forward<Args>(args)...);
            logMessage(LogLevel::OK, message);
        }
    }

private:
    Logger() = default;
    ~Logger() = default;
    
    // Disable copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Core logging implementation
    void logMessage(LogLevel level, const std::string& message);
    
    // Check if we should log at this level
    bool shouldLog(LogLevel level) const;
    
    // Get color code for log level
    std::string getColorCode(LogLevel level) const;
    
    // Get level prefix with proper padding
    std::string getLevelPrefix(LogLevel level) const;
    
    // Reset color code
    std::string getResetCode() const;
    
    // Check if terminal supports colors
    bool supportsColors() const;
    
    // Template helper for string formatting
    template<typename T>
    void formatMessageHelper(std::ostringstream& oss, const std::string& format, size_t pos, T&& value) {
        size_t nextPos = format.find("{}", pos);
        if (nextPos != std::string::npos) {
            oss << format.substr(pos, nextPos - pos) << value;
            oss << format.substr(nextPos + 2);
        } else {
            oss << format.substr(pos);
        }
    }
    
    template<typename T, typename... Args>
    void formatMessageHelper(std::ostringstream& oss, const std::string& format, size_t pos, T&& value, Args&&... args) {
        size_t nextPos = format.find("{}", pos);
        if (nextPos != std::string::npos) {
            oss << format.substr(pos, nextPos - pos) << value;
            formatMessageHelper(oss, format, nextPos + 2, std::forward<Args>(args)...);
        } else {
            oss << format.substr(pos);
        }
    }
    
    template<typename... Args>
    std::string formatMessage(const std::string& format, Args&&... args) {
        std::ostringstream oss;
        formatMessageHelper(oss, format, 0, std::forward<Args>(args)...);
        return oss.str();
    }
    
    // Specialization for no formatting arguments
    std::string formatMessage(const std::string& format) {
        return format;
    }
    
    // Member variables
    LogLevel m_currentLevel = LogLevel::INFO;
    bool m_debugMode = false;
    bool m_colorSupport = false;
    bool m_initialized = false;
};

// Convenience macros for easier usage
#define LOG_DEBUG(...) Logger::getInstance().debug(__VA_ARGS__)
#define LOG_INFO(...) Logger::getInstance().info(__VA_ARGS__)
#define LOG_IMPORTANT(...) Logger::getInstance().important(__VA_ARGS__)
#define LOG_WARN(...) Logger::getInstance().warn(__VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance().error(__VA_ARGS__)
#define LOG_SUCCESS(...) Logger::getInstance().success(__VA_ARGS__)