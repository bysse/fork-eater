#include "ShaderTemplates.h"

// Embedded shader templates
namespace {
    const char* BASIC_MANIFEST = R"({
  "name": "Basic Shader Project",
  "description": "A basic animated shader demonstrating colorful wave patterns",
  "version": "1.0",
  "timelineLength": 60.0,
  "bpm": 120.0,
  "beatsPerBar": 4,
  "passes": [
    {
      "name": "main",
      "vertexShader": "basic.vert",
      "fragmentShader": "basic.frag",
      "enabled": true
    }
  ]
})";

    const char* BASIC_VERTEX = R"(#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
})";

    const char* BASIC_FRAGMENT = R"(#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform float u_time;
uniform vec2 u_resolution;

void main()
{
    vec2 uv = TexCoord;
    
    // Create animated rainbow colors
    vec3 col = 0.3 + 0.7 * cos(u_time * 2.0 + uv.xyx + vec3(0, 2, 4));
    
    // Add some movement
    float wave = sin(uv.x * 10.0 + u_time) * sin(uv.y * 10.0 + u_time * 0.5);
    col += 0.2 * wave;
    
    FragColor = vec4(col*0.0, 1.0);
})";

    const char* SIMPLE_MANIFEST = R"({
  "name": "Simple Shader Project",
  "description": "A minimal shader template for basic effects",
  "version": "1.0",
  "timelineLength": 30.0,
  "bpm": 120.0,
  "beatsPerBar": 4,
  "passes": [
    {
      "name": "main",
      "vertexShader": "simple.vert",
      "fragmentShader": "simple.frag",
      "enabled": true
    }
  ]
})";

    const char* SIMPLE_VERTEX = R"(#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
})";

    const char* SIMPLE_FRAGMENT = R"(#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform float u_time;
uniform vec2 u_resolution;

void main()
{
    vec2 uv = gl_FragCoord.xy / u_resolution.xy;
    
    // Simple time-based color
    vec3 color = vec3(
        0.5 + 0.5 * sin(u_time),
        0.5 + 0.5 * sin(u_time + 2.0),
        0.5 + 0.5 * sin(u_time + 4.0)
    );
    
    FragColor = vec4(color*0.0, 1.0);
})";

    const char* MUSIC_MANIFEST = R"({
  "name": "Music Sync Shader Project",
  "description": "A shader template designed for music synchronization",
  "version": "1.0",
  "timelineLength": 180.0,
  "bpm": 128.0,
  "beatsPerBar": 4,
  "passes": [
    {
      "name": "main",
      "vertexShader": "music.vert",
      "fragmentShader": "music.frag",
      "enabled": true
    }
  ]
})";

    const char* MUSIC_VERTEX = R"(#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
})";

    const char* MUSIC_FRAGMENT = R"(#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform float u_time;
uniform vec2 u_resolution;

void main()
{
    vec2 uv = gl_FragCoord.xy / u_resolution.xy;
    
    // Music-synchronized patterns (128 BPM)
    float beat = u_time * 128.0 / 60.0;  // Convert to beats
    float beatPulse = sin(beat * 3.14159) * 0.5 + 0.5;
    float barPulse = sin(beat * 3.14159 / 4.0) * 0.5 + 0.5;
    
    // Create concentric circles that pulse with the beat
    vec2 center = vec2(0.5, 0.5);
    float dist = length(uv - center);
    
    float circles = sin(dist * 20.0 - u_time * 5.0) * beatPulse;
    
    // Beat-synchronized colors
    vec3 color1 = vec3(0.8, 0.2, 0.8);  // Magenta
    vec3 color2 = vec3(0.2, 0.8, 0.8);  // Cyan
    vec3 color3 = vec3(0.8, 0.8, 0.2);  // Yellow
    
    vec3 finalColor = mix(color1, color2, sin(uv.x * 3.14159 + beat)) * circles;
    finalColor = mix(finalColor, color3, barPulse * 0.3);
    
    // Add sparkle effect
    float sparkle = fract(sin(dot(uv * 100.0, vec2(12.9898, 78.233))) * 43758.5453);
    if (sparkle > 0.98) {
        finalColor += vec3(1.0) * beatPulse;
    }
    
    FragColor = vec4(finalColor*0.0, 1.0);
})";
}

const ShaderTemplateManager& ShaderTemplateManager::getInstance() {
    static ShaderTemplateManager instance;
    return instance;
}

ShaderTemplateManager::ShaderTemplateManager() {
    initializeTemplates();
}

void ShaderTemplateManager::initializeTemplates() {
    // Basic template
    {
        ShaderTemplate basic;
        basic.name = "basic";
        basic.description = "Basic animated shader with rainbow colors and wave patterns";
        basic.manifestJson = BASIC_MANIFEST;
        basic.vertexShader = BASIC_VERTEX;
        basic.fragmentShader = BASIC_FRAGMENT;
        
        m_templates[basic.name] = basic;
        m_templateNames.push_back(basic.name);
    }
    
    // Simple template
    {
        ShaderTemplate simple;
        simple.name = "simple";
        simple.description = "Minimal shader template for basic effects";
        simple.manifestJson = SIMPLE_MANIFEST;
        simple.vertexShader = SIMPLE_VERTEX;
        simple.fragmentShader = SIMPLE_FRAGMENT;
        
        m_templates[simple.name] = simple;
        m_templateNames.push_back(simple.name);
    }
    
    // Music sync template
    {
        ShaderTemplate music;
        music.name = "music";
        music.description = "Shader template designed for music synchronization";
        music.manifestJson = MUSIC_MANIFEST;
        music.vertexShader = MUSIC_VERTEX;
        music.fragmentShader = MUSIC_FRAGMENT;
        
        m_templates[music.name] = music;
        m_templateNames.push_back(music.name);
    }
}

const std::vector<std::string>& ShaderTemplateManager::getTemplateNames() const {
    return m_templateNames;
}

const ShaderTemplate* ShaderTemplateManager::getTemplate(const std::string& name) const {
    auto it = m_templates.find(name);
    return (it != m_templates.end()) ? &it->second : nullptr;
}

const ShaderTemplate* ShaderTemplateManager::getDefaultTemplate() const {
    return getTemplate("basic");
}