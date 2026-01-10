#include "ParameterPanel.h"
#include "imgui.h"
#include "Settings.h"
#include "RenderScaleMode.h"

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
                std::vector<float> values;
                switch (uniform.type) {
                    case GL_FLOAT:
                        values.assign(uniform.value, uniform.value + 1);
                        break;
                    case GL_FLOAT_VEC2:
                        values.assign(uniform.value, uniform.value + 2);
                        break;
                    case GL_FLOAT_VEC3:
                        values.assign(uniform.value, uniform.value + 3);
                        break;
                    case GL_FLOAT_VEC4:
                        values.assign(uniform.value, uniform.value + 4);
                        break;
                }
                m_shaderProject->getUniformValues()[shaderName][uniform.name] = values;
                m_shaderProject->saveState(m_shaderManager);
            }
        }

        if (!shader->switchFlags.empty()) {
            ImGui::Separator();
            for (const auto& flag : shader->switchFlags) {
                bool enabled = m_shaderManager->getSwitchState(flag);
                if (ImGui::Checkbox(flag.c_str(), &enabled)) {
                    m_shaderManager->setSwitchState(flag, enabled);
                    RenderScaleMode scaleMode = Settings::getInstance().getRenderScaleMode();
                    if (m_shaderManager->reloadShader(shaderName, scaleMode)) {
                        auto newShader = m_shaderManager->getShader(shaderName);
                        if (newShader && m_shaderProject) {
                            m_shaderProject->applyUniformsToShader(shaderName, newShader);
                            m_shaderProject->saveState(m_shaderManager);
                        }
                    }
                }
            }
        }
    } else {
        ImGui::Text("No parameters found.");
    }
}

void ParameterPanel::setProject(std::shared_ptr<ShaderProject> shaderProject) {
    m_shaderProject = shaderProject;
}
