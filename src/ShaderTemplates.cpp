#include "ShaderTemplates.h"
#include "GeneratedShaderTemplates.h"
#include "json.hpp"

using json = nlohmann::json;

ShaderTemplateManager& ShaderTemplateManager::getInstance() {
    static ShaderTemplateManager instance;
    return instance;
}

ShaderTemplateManager::ShaderTemplateManager() {
    initializeTemplates();
}

void ShaderTemplateManager::initializeTemplates() {
    EmbeddedTemplates::initialize();

    for (auto const& [path, data] : EmbeddedTemplates::g_templates) {
        if (path.find("manifest.json") != std::string::npos) {
            std::string templateName = path.substr(10, path.find_last_of('/') - 10);

            json manifest = json::parse(data.first, data.first + data.second);

            ShaderTemplate newTemplate;
            newTemplate.name = templateName;
            newTemplate.description = manifest["description"];

            std::string vertPath = "templates/" + templateName + "/" + templateName + ".vert";
            std::string fragPath = "templates/" + templateName + "/" + templateName + ".frag";

            newTemplate.manifestJson = data.first;
            newTemplate.manifestJsonSize = data.second;
            newTemplate.vertexShader = EmbeddedTemplates::g_templates[vertPath].first;
            newTemplate.vertexShaderSize = EmbeddedTemplates::g_templates[vertPath].second;
            newTemplate.fragmentShader = EmbeddedTemplates::g_templates[fragPath].first;
            newTemplate.fragmentShaderSize = EmbeddedTemplates::g_templates[fragPath].second;

            m_templates[templateName] = newTemplate;
            m_templateNames.push_back(templateName);
        }
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
    if (m_templateNames.empty()) {
        return nullptr;
    }
    return getTemplate(m_templateNames.front());
}