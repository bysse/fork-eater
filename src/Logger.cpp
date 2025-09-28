#include "Logger.h"
#include <unistd.h>
#include <cstdlib>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(bool debugMode) {
    m_debugMode = debugMode;
    m_colorSupport = supportsColors();
    m_initialized = true;
    
    // Set log level based on debug mode
    if (debugMode) {
        m_currentLevel = LogLevel::DEBUG;
    } else {
        m_currentLevel = LogLevel::INFO;
    }
}

void Logger::setLogLevel(LogLevel level) {
    m_currentLevel = level;
}

LogLevel Logger::getLogLevel() const {
    return m_currentLevel;
}

bool Logger::isDebugEnabled() const {
    return m_debugMode;
}

void Logger::debug(const std::string& message) {
    if (shouldLog(LogLevel::DEBUG)) {
        logMessage(LogLevel::DEBUG, message);
    }
}

void Logger::info(const std::string& message) {
    if (shouldLog(LogLevel::INFO)) {
        logMessage(LogLevel::INFO, message);
    }
}

void Logger::warn(const std::string& message) {
    if (shouldLog(LogLevel::WARN)) {
        logMessage(LogLevel::WARN, message);
    }
}

void Logger::error(const std::string& message) {
    if (shouldLog(LogLevel::ERROR)) {
        logMessage(LogLevel::ERROR, message);
    }
}

void Logger::success(const std::string& message) {
    if (shouldLog(LogLevel::OK)) {
        logMessage(LogLevel::OK, message);
    }
}

void Logger::important(const std::string& message) {
    if (shouldLog(LogLevel::IMPORTANT)) {
        logMessage(LogLevel::IMPORTANT, message);
    }
}

void Logger::logMessage(LogLevel level, const std::string& message) {
    std::ostream& output = (level == LogLevel::ERROR) ? std::cerr : std::cout;
    
    if (m_colorSupport) {
        output << getColorCode(level);
    }
    
    output << getLevelPrefix(level) << " " << message;
    
    if (m_colorSupport) {
        output << getResetCode();
    }
    
    output << std::endl;
}

bool Logger::shouldLog(LogLevel level) const {
    if (!m_initialized) {
        return true; // Log everything if not initialized yet
    }
    
    return static_cast<int>(level) >= static_cast<int>(m_currentLevel);
}

std::string Logger::getColorCode(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:
            return "\033[37m";    // White
        case LogLevel::INFO:
            return "\033[90m";    // Gray/Dark Gray  
        case LogLevel::IMPORTANT:
            return "\033[37m";    // White
        case LogLevel::WARN:
            return "\033[33m";    // Yellow
        case LogLevel::ERROR:
            return "\033[31m";    // Red
        case LogLevel::OK:
            return "\033[32m";    // Green
        default:
            return "";
    }
}

std::string Logger::getLevelPrefix(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:
            return "[DEBUG]";
        case LogLevel::INFO:
            return "[INFO ]";
        case LogLevel::IMPORTANT:
            return "[INFO ]";
        case LogLevel::WARN:
            return "[WARN ]";
        case LogLevel::ERROR:
            return "[ERROR]";
        case LogLevel::OK:
            return "[OK   ]";
        default:
            return "[UNKNOWN]";
    }
}

std::string Logger::getResetCode() const {
    return "\033[0m";
}

bool Logger::supportsColors() const {
    // Check if we're writing to a terminal and if color is supported
    if (!isatty(STDOUT_FILENO)) {
        return false;
    }
    
    // Check environment variables that indicate color support
    const char* term = std::getenv("TERM");
    const char* colorterm = std::getenv("COLORTERM");
    
    if (colorterm != nullptr) {
        return true;
    }
    
    if (term != nullptr) {
        std::string termStr(term);
        if (termStr.find("color") != std::string::npos ||
            termStr.find("xterm") != std::string::npos ||
            termStr.find("screen") != std::string::npos ||
            termStr == "linux") {
            return true;
        }
    }
    
    return false;
}