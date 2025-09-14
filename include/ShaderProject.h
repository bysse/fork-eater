#pragma once

#include <string>
#include <vector>
#include <memory>

// Forward declarations
class ShaderManager;

struct ShaderPass {
    std::string name;
    std::string vertexShader;
    std::string fragmentShader;
    std::vector<std::string> inputs;  // For multi-pass rendering
    std::string output;               // Output buffer name (optional)
    bool enabled = true;
};

struct ShaderProjectManifest {
    std::string name;
    std::string description;
    std::vector<ShaderPass> passes;
    float timelineLength = 120.0f;  // In seconds
    float bpm = 120.0f;             // Beats per minute
    int beatsPerBar = 4;            // For timeline display
    std::string version = "1.0";
    
    // Calculated properties
    float getBeatsPerSecond() const { return bpm / 60.0f; }
    float getBarsPerSecond() const { return getBeatsPerSecond() / beatsPerBar; }
    float getTotalBeats() const { return timelineLength * getBeatsPerSecond(); }
    float getTotalBars() const { return timelineLength * getBarsPerSecond(); }
    
    // Convert between time and beats
    float secondsToBeats(float seconds) const { return seconds * getBeatsPerSecond(); }
    float beatsToSeconds(float beats) const { return beats / getBeatsPerSecond(); }
    float secondsToBars(float seconds) const { return seconds * getBarsPerSecond(); }
    float barsToSeconds(float bars) const { return bars / getBarsPerSecond(); }
};

class ShaderProject {
public:
    ShaderProject();
    ~ShaderProject();
    
    // Project loading/saving
    bool loadFromDirectory(const std::string& projectPath);
    bool saveToDirectory(const std::string& projectPath) const;
    
    // Create new project
    bool createNew(const std::string& projectPath, const std::string& name);
    
    // Manifest access
    const ShaderProjectManifest& getManifest() const { return m_manifest; }
    ShaderProjectManifest& getManifest() { return m_manifest; }
    
    // Project properties
    const std::string& getProjectPath() const { return m_projectPath; }
    bool isLoaded() const { return m_isLoaded; }
    
    // Shader pass management
    const std::vector<ShaderPass>& getPasses() const { return m_manifest.passes; }
    void addPass(const ShaderPass& pass);
    void removePass(size_t index);
    void movePass(size_t from, size_t to);
    
    // File paths
    std::string getManifestPath() const;
    std::string getShaderPath(const std::string& filename) const;
    std::string getAssetsPath() const;
    
    // Validation
    bool validateProject() const;
    std::vector<std::string> getValidationErrors() const;
    
    // Load shaders into ShaderManager
    bool loadShadersIntoManager(std::shared_ptr<ShaderManager> shaderManager) const;

private:
    ShaderProjectManifest m_manifest;
    std::string m_projectPath;
    bool m_isLoaded;
    
    // Private methods
    bool loadManifest();
    bool saveManifest() const;
    bool parseManifestJson(const std::string& jsonContent);
    std::string generateManifestJson() const;
    bool createDirectoryStructure() const;
    
    // Validation helpers
    bool validateManifest() const;
    bool validateShaderFiles() const;
    void parsePassesFromJson(const std::string& jsonContent);
    ShaderPass parsePassObject(const std::string& passJson);
    void createDefaultShaders() const;
};