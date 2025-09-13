#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <string>

// Forward declare GLFW types
struct GLFWwindow;

// Modifier flags
enum class KeyModifier : int {
    None = 0,
    Ctrl = 1,
    Shift = 2,
    Alt = 4,
    CtrlShift = Ctrl | Shift,
    CtrlAlt = Ctrl | Alt,
    ShiftAlt = Shift | Alt,
    CtrlShiftAlt = Ctrl | Shift | Alt
};

// Shortcut action callback
using ShortcutCallback = std::function<void()>;

// Shortcut description for help display
struct ShortcutInfo {
    std::string keys;
    std::string description;
    std::string category;
};

// Internal shortcut key representation
struct ShortcutKey {
    int glfwKey;
    KeyModifier modifiers;
    
    bool operator==(const ShortcutKey& other) const {
        return glfwKey == other.glfwKey && modifiers == other.modifiers;
    }
};

// Custom hash for ShortcutKey
struct ShortcutKeyHash {
    std::size_t operator()(const ShortcutKey& k) const {
        return std::hash<int>()(k.glfwKey) ^ 
               (std::hash<int>()(static_cast<int>(k.modifiers)) << 1);
    }
};

class ShortcutManager {
public:
    ShortcutManager();
    ~ShortcutManager();
    
    // Register a shortcut
    void registerShortcut(int glfwKey, KeyModifier modifiers, 
                         ShortcutCallback callback,
                         const std::string& keyDescription, 
                         const std::string& description,
                         const std::string& category = "General");
    
    // Handle key press events
    bool handleKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods);
    
    // Get all registered shortcuts for help display
    std::vector<ShortcutInfo> getAllShortcuts() const;
    
    // Get shortcuts by category
    std::vector<ShortcutInfo> getShortcutsByCategory(const std::string& category) const;
    
    // Clear all shortcuts
    void clearShortcuts();

private:
    std::unordered_map<ShortcutKey, ShortcutCallback, ShortcutKeyHash> m_shortcuts;
    std::unordered_map<ShortcutKey, ShortcutInfo, ShortcutKeyHash> m_shortcutInfo;
    
    // Convert GLFW modifiers to our KeyModifier enum
    KeyModifier convertGlfwModifiers(int glfwMods) const;
    
    // Convert KeyModifier enum to string
    std::string modifierToString(KeyModifier mod) const;
    
    // Convert GLFW key code to string
    std::string keyToString(int glfwKey) const;
};