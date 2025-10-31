#include "ShaderTemplates.h"
#include "GeneratedShaderTemplates.h"
#include "json.hpp"

#include <algorithm>
#include <string>

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

    const std::string prefix = "templates/";

    for (auto const& [path, data] : EmbeddedTemplates::g_templates) {
        if (path.compare(0, prefix.size(), prefix) != 0) {
            continue;
        }

        std::string relativePath = path.substr(prefix.size());
        auto separatorPos = relativePath.find('/');
        if (separatorPos == std::string::npos) {
            continue;
        }

        std::string templateName = relativePath.substr(0, separatorPos);
        std::string fileName = relativePath.substr(separatorPos + 1);

        auto& shaderTemplate = m_templates[templateName];
        shaderTemplate.name = templateName;

        if (fileName == "manifest.json") {
            shaderTemplate.manifestJson = data.first;
            shaderTemplate.manifestJsonSize = data.second;

            try {
                json manifest = json::parse(data.first, data.first + data.second);
                shaderTemplate.description = manifest.value("description", "");
            } catch (const json::parse_error&) {
                shaderTemplate.description.clear();
            }
        } else {
            shaderTemplate.files[fileName] = data;
        }
    }

    for (auto it = m_templates.begin(); it != m_templates.end(); ) {
        if (it->second.manifestJson == nullptr) {
            it = m_templates.erase(it);
            continue;
        }
        ++it;
    }

    m_templateNames.reserve(m_templates.size());
    for (const auto& [name, shaderTemplate] : m_templates) {
        (void)shaderTemplate;
        m_templateNames.push_back(name);
    }
    std::sort(m_templateNames.begin(), m_templateNames.end());
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
