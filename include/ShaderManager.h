#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <vector>
#include <map>

#include "ShaderPreprocessor.h"
#include "RenderScaleMode.h"

// Forward declare OpenGL types
typedef unsigned int GLuint;
typedef unsigned int GLenum;

#include "Framebuffer.h"

class ShaderPreprocessor;

struct ShaderUniform {
    std::string name;
    GLenum type;
    float value[4];
    float min = 0.0f;
    float max = 1.0f;
    std::string label;
    std::string group;
};

struct ShaderBuffer {
    std::string name;
    std::string type = "samplerBuffer"; // "samplerBuffer" or "ubo"
    std::string dataType = "float"; // "float", "vec2", "vec3", "vec4"
    std::vector<float> data; // For in-line data
    std::string file; // For external CSV/data files
    bool striped = false; // Whether the data is striped (planar/SoA)
    int precision = -1; // Number of mantissa bits to keep (-1 = full precision)
    
    // Fixed point configuration
    bool fixedPoint = false;
    int fixedPointM = 0;
    int fixedPointN = 0;
    bool fixedPointSigned = true;
    std::string fixedPointFormat = "";

    // Export configuration
    bool hasExport = false;
    std::string exportFormat = "";
    std::string exportOutputFile = "";
    bool exportStripedCoordinates = false;
    bool exportStripedBytes = false;
};

class ShaderManager {
public:
    struct ShaderProgram {
        GLuint programId;
        GLuint vertexShaderId;
        GLuint fragmentShaderId;
        std::string vertexPath;
        std::string fragmentPath;
        std::string preprocessedVertexSource;
        std::string preprocessedFragmentSource;
        std::vector<ShaderPreprocessor::LineMapping> vertexLineMappings;
        std::vector<ShaderPreprocessor::LineMapping> fragmentLineMappings;
        std::vector<std::string> includedFiles;
        std::vector<ShaderUniform> uniforms;
        std::vector<ShaderPreprocessor::SwitchInfo> switchFlags;
        std::vector<ShaderPreprocessor::SliderInfo> sliders;
        std::vector<ShaderPreprocessor::LabelInfo> labels;
        std::map<std::string, GLenum> systemUniformTypes;
        std::string lastError;
        bool isValid;
    };

    ShaderManager();
    ~ShaderManager();

    // Load and compile shader program
    std::shared_ptr<ShaderProgram> loadShader(const std::string& name, 
                                               const std::string& vertexPath, 
                                               const std::string& fragmentPath,
                                               RenderScaleMode scaleMode = RenderScaleMode::Resolution);
    
    // Reload existing shader
    bool reloadShader(const std::string& name, RenderScaleMode scaleMode = RenderScaleMode::Resolution);
    
    // Update data buffers
    void updateBuffers(const std::vector<ShaderBuffer>& buffers);

    // Get shader program
    std::shared_ptr<ShaderProgram> getShader(const std::string& name);
    
    // Use shader program
    void useShader(const std::string& name);
    
    // Render to framebuffer
    void renderToFramebuffer(const std::string& name, int width, int height, float time, float renderScaleFactor, RenderScaleMode scaleMode);

    // Get texture ID of a framebuffer
    GLuint getFramebufferTexture(const std::string& name);

    // Get the UV scale of the framebuffer texture (useful when rendering a sub-region)
    std::pair<float, float> getFramebufferUVScale(const std::string& name);

    // Set uniform helpers
    void setUniform(const std::string& name, float value);
    void setUniform(const std::string& name, const float* value, int count);
    void setUniform(const std::string& name, int value);
    void setUniform(const std::string& name, bool value);

    // Mouse input helpers
    void setMousePosition(float x, float y);
    void setMouseClickState(bool clicked);
    void updateIntegratedMouse(float dx, float dy);
    const float* getIntegratedMouse() const { return m_mouseIntegrated; }
    
    // Camera helpers
    void setCameraPosition(float x, float y, float z);
    const float* getCameraPosition() const { return m_camPos; }
    void setCameraTarget(float x, float y, float z);
    const float* getCameraTarget() const { return m_camTarget; }
    
    // Get all shader names
    std::vector<std::string> getShaderNames() const;

    // Get current shader name
    std::string getCurrentShader() const;
    
    // Clear all loaded shaders
    void clearShaders();
    
    // Set compilation callback
    void setCompilationCallback(std::function<void(const std::string&, bool, const std::string&)> callback);

    // Get preprocessed shader source
    std::string getPreprocessedSource(const std::string& name, bool fragment = true);

    // Switch state management
    bool getSwitchState(const std::string& name) const;
    void setSwitchState(const std::string& name, bool enabled);
    const std::unordered_map<std::string, bool>& getSwitchStates() const;

    // Slider state management (compile-time sliders)
    float getSliderState(const std::string& name) const;
    void setSliderState(const std::string& name, float value);
    const std::unordered_map<std::string, float>& getSliderStates() const;

private:
    struct InternalBuffer {
        GLuint tbo;
        GLuint texture;
        size_t size;
        std::string bufferType; // "samplerBuffer" or "ubo"
        std::string dataType; // "float", "vec2", "vec3", "vec4"
        std::vector<float> lastData;
    };

    std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> m_shaders;
    std::unordered_map<std::string, std::unique_ptr<Framebuffer>> m_framebuffers;
    std::unordered_map<std::string, std::pair<float, float>> m_framebufferScales;
    std::unordered_map<std::string, InternalBuffer> m_buffers;
    std::string m_currentShader;
    std::function<void(const std::string&, bool, const std::string&)> m_compilationCallback;
    GLuint m_quadVAO;
    GLuint m_quadVBO;
    std::unordered_map<std::string, bool> m_errorLogged;
    std::unordered_map<std::string, bool> m_switchStates;
    std::unordered_map<std::string, float> m_sliderStates;
    float m_mouseUniform[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float m_mouseIntegrated[2] = {0.0f, 0.0f}; // Start at center
    float m_camPos[3] = {0.0f, 0.5f, 2.0f}; // Default camera position
    float m_camTarget[3] = {0.0f, 0.0f, 0.0f}; // Default camera target
    
    // Helper functions
    GLuint compileShader(const std::string& source, GLenum shaderType, std::string& outErrorLog, const std::vector<ShaderPreprocessor::LineMapping>* lineMappings = nullptr);
    GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader, std::string& outErrorLog);
    std::string readFile(const std::string& filePath);
    std::string getShaderInfoLog(GLuint shader);
    std::string getProgramInfoLog(GLuint program);
    void cleanupShader(ShaderProgram& shader);
    std::string remapErrorLog(const std::string& log, const std::vector<ShaderPreprocessor::LineMapping>* lineMappings) const;
    
    ShaderPreprocessor* m_preprocessor;

    // Internal resources for upscaling
    struct {
        GLuint programId;
        GLuint textureLocation;
    } m_simpleTextureProgram;

    void setupSimpleTextureProgram();
    void performUpscale(const std::string& name, int width, int height, float scaleX, float scaleY);
};
