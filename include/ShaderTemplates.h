#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

struct ShaderTemplate {
    std::string name;
    std::string description;
    const char* manifestJson = nullptr;
    size_t manifestJsonSize = 0;
    std::unordered_map<std::string, std::pair<const char*, size_t>> files;
};

class ShaderTemplateManager {
public:
    static ShaderTemplateManager& getInstance();

    const std::vector<std::string>& getTemplateNames() const;
    const ShaderTemplate* getTemplate(const std::string& name) const;
    const ShaderTemplate* getDefaultTemplate() const;

private:
    ShaderTemplateManager();
    void initializeTemplates();

    std::vector<std::string> m_templateNames;
    std::unordered_map<std::string, ShaderTemplate> m_templates;
};
