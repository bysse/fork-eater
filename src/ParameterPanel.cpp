#include "ParameterPanel.h"
#include "imgui.h"

ParameterPanel::ParameterPanel(std::shared_ptr<ShaderManager> shaderManager, std::shared_ptr<ShaderProject> shaderProject)
    : m_shaderManager(shaderManager), m_shaderProject(shaderProject) {}

void ParameterPanel::render(const std::string& shaderName) {
    auto shader = m_shaderManager->getShader(shaderName);
    if (shader && !shader->uniforms.empty()) {
        for (auto& uniform : shader->uniforms) {
            // Skip system uniforms
            if (uniform.name == "iTime" || uniform.name == "iResolution" || uniform.name == "iMouse") {
                continue;
            }

            bool valueChanged = false;
            switch (uniform.type) {
                case GL_FLOAT:
                    valueChanged = ImGui::SliderFloat(uniform.name.c_str(), uniform.value, 0.0f, 1.0f);
                    break;
                case GL_FLOAT_VEC2:
                    valueChanged = ImGui::SliderFloat2(uniform.name.c_str(), uniform.value, 0.0f, 1.0f);
                    break;
                case GL_FLOAT_VEC3:
                    valueChanged = ImGui::SliderFloat3(uniform.name.c_str(), uniform.value, 0.0f, 1.0f);
                    break;
                case GL_FLOAT_VEC4:
                    valueChanged = ImGui::SliderFloat4(uniform.name.c_str(), uniform.value, 0.0f, 1.0f);
                    break;
            }

            if (valueChanged && m_shaderProject) {
                std::vector<float> values(std::begin(uniform.value), std::end(uniform.value));
                m_shaderProject->getUniformValues()[shaderName][uniform.name] = values;
            }
        }

        if (!shader->switchFlags.empty()) {
            ImGui::Separator();
            for (const auto& flag : shader->switchFlags) {
                bool enabled = m_shaderManager->getSwitchState(flag);
                if (ImGui::Checkbox(flag.c_str(), &enabled)) {
                    m_shaderManager->setSwitchState(flag, enabled);
                    m_shaderManager->reloadShader(shaderName);
                }
            }
        }
    } else {
        ImGui::Text("No parameters found.");
    }
}
