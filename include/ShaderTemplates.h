#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

struct ShaderTemplate {
    std::string name;
    std::string description;
    const char* manifestJson;
    size_t manifestJsonSize;
    const char* vertexShader;
    size_t vertexShaderSize;
    const char* fragmentShader;
    size_t fragmentShaderSize;
};

class ShaderTemplateManager {
public:
    static ShaderTemplateManager& getInstance();

    const std::vector<std::string>& getTemplateNames() const;
    const ShaderTemplate* getTemplate(const std::string& name) const;
    const ShaderTemplate* getDefaultTemplate() const;

    void registerTemplate(const std::string& name, const std::string& description,
                          const std::pair<const unsigned char*, size_t>& manifest,
                          const std::pair<const unsigned char*, size_t>& vertex,
                          const std::pair<const unsigned char*, size_t>& fragment);

private:
    ShaderTemplateManager();
    void initializeTemplates();

    std::vector<std::string> m_templateNames;
    std::unordered_map<std::string, ShaderTemplate> m_templates;
};