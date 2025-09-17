#pragma once

#include <string>
#include <vector>
#include <unordered_map>

struct ShaderTemplate {
    std::string name;
    std::string description;
    std::string manifestJson;
    std::string vertexShader;
    std::string fragmentShader;
};

class ShaderTemplateManager {
public:
    static const ShaderTemplateManager& getInstance();
    
    const std::vector<std::string>& getTemplateNames() const;
    const ShaderTemplate* getTemplate(const std::string& name) const;
    const ShaderTemplate* getDefaultTemplate() const;
    
private:
    ShaderTemplateManager();
    void initializeTemplates();
    
    std::vector<std::string> m_templateNames;
    std::unordered_map<std::string, ShaderTemplate> m_templates;
};