#include "ShortcutManager.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <sstream>

ShortcutManager::ShortcutManager() {
    // Constructor
}

ShortcutManager::~ShortcutManager() {
    // Destructor
}

void ShortcutManager::registerShortcut(int glfwKey, KeyModifier modifiers, 
                                     ShortcutCallback callback,
                                     const std::string& keyDescription, 
                                     const std::string& description,
                                     const std::string& category) {
    ShortcutKey key = {glfwKey, modifiers};
    m_shortcuts[key] = callback;
    m_shortcutInfo[key] = {keyDescription, description, category};
}

bool ShortcutManager::handleKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) {
        return false; // Only handle key press, not release
    }
    
    KeyModifier modifiers = convertGlfwModifiers(mods);
    ShortcutKey shortcutKey = {key, modifiers};
    
    auto it = m_shortcuts.find(shortcutKey);
    if (it != m_shortcuts.end()) {
        it->second(); // Execute the callback
        return true; // Key was handled
    }
    
    return false; // Key was not handled
}

std::vector<ShortcutInfo> ShortcutManager::getAllShortcuts() const {
    std::vector<ShortcutInfo> shortcuts;
    for (const auto& pair : m_shortcutInfo) {
        shortcuts.push_back(pair.second);
    }
    
    // Sort by category, then by keys
    std::sort(shortcuts.begin(), shortcuts.end(), 
        [](const ShortcutInfo& a, const ShortcutInfo& b) {
            if (a.category != b.category) {
                return a.category < b.category;
            }
            return a.keys < b.keys;
        });
    
    return shortcuts;
}

std::vector<ShortcutInfo> ShortcutManager::getShortcutsByCategory(const std::string& category) const {
    std::vector<ShortcutInfo> shortcuts;
    for (const auto& pair : m_shortcutInfo) {
        if (pair.second.category == category) {
            shortcuts.push_back(pair.second);
        }
    }
    
    // Sort by keys
    std::sort(shortcuts.begin(), shortcuts.end(), 
        [](const ShortcutInfo& a, const ShortcutInfo& b) {
            return a.keys < b.keys;
        });
    
    return shortcuts;
}

void ShortcutManager::clearShortcuts() {
    m_shortcuts.clear();
    m_shortcutInfo.clear();
}

KeyModifier ShortcutManager::convertGlfwModifiers(int glfwMods) const {
    int result = static_cast<int>(KeyModifier::None);
    
    if (glfwMods & GLFW_MOD_CONTROL) {
        result |= static_cast<int>(KeyModifier::Ctrl);
    }
    if (glfwMods & GLFW_MOD_SHIFT) {
        result |= static_cast<int>(KeyModifier::Shift);
    }
    if (glfwMods & GLFW_MOD_ALT) {
        result |= static_cast<int>(KeyModifier::Alt);
    }
    
    return static_cast<KeyModifier>(result);
}

std::string ShortcutManager::modifierToString(KeyModifier mod) const {
    std::vector<std::string> parts;
    
    if (static_cast<int>(mod) & static_cast<int>(KeyModifier::Ctrl)) {
        parts.push_back("Ctrl");
    }
    if (static_cast<int>(mod) & static_cast<int>(KeyModifier::Shift)) {
        parts.push_back("Shift");
    }
    if (static_cast<int>(mod) & static_cast<int>(KeyModifier::Alt)) {
        parts.push_back("Alt");
    }
    
    if (parts.empty()) {
        return "";
    }
    
    std::string result;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += " + ";
        result += parts[i];
    }
    
    return result;
}

std::string ShortcutManager::keyToString(int glfwKey) const {
    switch (glfwKey) {
        case GLFW_KEY_SPACE: return "Space";
        case GLFW_KEY_LEFT: return "Left Arrow";
        case GLFW_KEY_RIGHT: return "Right Arrow";
        case GLFW_KEY_UP: return "Up Arrow";
        case GLFW_KEY_DOWN: return "Down Arrow";
        case GLFW_KEY_HOME: return "Home";
        case GLFW_KEY_END: return "End";
        case GLFW_KEY_ESCAPE: return "Escape";
        case GLFW_KEY_ENTER: return "Enter";
        case GLFW_KEY_TAB: return "Tab";
        case GLFW_KEY_BACKSPACE: return "Backspace";
        case GLFW_KEY_DELETE: return "Delete";
        case GLFW_KEY_F1: return "F1";
        case GLFW_KEY_F2: return "F2";
        case GLFW_KEY_F3: return "F3";
        case GLFW_KEY_F4: return "F4";
        case GLFW_KEY_F5: return "F5";
        case GLFW_KEY_F6: return "F6";
        case GLFW_KEY_F7: return "F7";
        case GLFW_KEY_F8: return "F8";
        case GLFW_KEY_F9: return "F9";
        case GLFW_KEY_F10: return "F10";
        case GLFW_KEY_F11: return "F11";
        case GLFW_KEY_F12: return "F12";
        default:
            // For regular keys, try to convert to char
            if (glfwKey >= GLFW_KEY_A && glfwKey <= GLFW_KEY_Z) {
                return std::string(1, 'A' + (glfwKey - GLFW_KEY_A));
            }
            if (glfwKey >= GLFW_KEY_0 && glfwKey <= GLFW_KEY_9) {
                return std::string(1, '0' + (glfwKey - GLFW_KEY_0));
            }
            return "Unknown";
    }
}