#include "ParameterPanel.h"
#include "imgui.h"
#include "Settings.h"
#include "RenderScaleMode.h"

ParameterPanel::ParameterPanel(std::shared_ptr<ShaderManager> shaderManager, std::shared_ptr<ShaderProject> shaderProject)
    : m_shaderManager(shaderManager), m_shaderProject(shaderProject) {}

void ParameterPanel::render(const std::string& shaderName) {
    auto shader = m_shaderManager->getShader(shaderName);
    if (!shader) {
        ImGui::Text("Shader not found.");
        return;
    }

    bool hasUniforms = false;
    for (const auto& uniform : shader->uniforms) {
        if (uniform.name != "iTime" && uniform.name != "iResolution" && uniform.name != "iMouse") {
            hasUniforms = true;
            break;
        }
    }

    if (!hasUniforms && shader->switchFlags.empty()) {
        ImGui::Text("No parameters found.");
        return;
    }

    if (hasUniforms) {
        std::string currentGroup = "";
        bool inGroup = false;

        // Group uniforms by group name, but preserve order as best as possible?
        // Actually, uniforms are stored in order of appearance.
        // If we just iterate, we will encounter items in different groups if they are interleaved.
        // But usually, grouped items are contiguous.
        
        for (auto& uniform : shader->uniforms) {
            // Skip system uniforms
            if (uniform.name == "iTime" || uniform.name == "iResolution" || uniform.name == "iMouse") {
                continue;
            }

            // Check if group changed
            if (uniform.group != currentGroup) {
                if (inGroup) {
                    ImGui::Unindent();
                    inGroup = false;
                }
                
                currentGroup = uniform.group;
                
                if (!currentGroup.empty()) {
                    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", currentGroup.c_str());
                    ImGui::Indent();
                    inGroup = true;
                }
            }

            std::string label = uniform.label.empty() ? uniform.name : uniform.label;
            bool valueChanged = false;
            switch (uniform.type) {
                case GL_FLOAT:
                    valueChanged = ImGui::SliderFloat(label.c_str(), uniform.value, uniform.min, uniform.max);
                    break;
                case GL_FLOAT_VEC2:
                    valueChanged = ImGui::SliderFloat2(label.c_str(), uniform.value, uniform.min, uniform.max);
                    break;
                case GL_FLOAT_VEC3:
                    valueChanged = ImGui::SliderFloat3(label.c_str(), uniform.value, uniform.min, uniform.max);
                    break;
                case GL_FLOAT_VEC4:
                    valueChanged = ImGui::SliderFloat4(label.c_str(), uniform.value, uniform.min, uniform.max);
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
        
        if (inGroup) {
            ImGui::Unindent();
        }
    }

    if (!shader->sliders.empty()) {
        if (hasUniforms) ImGui::Separator();
        
        ImGui::TextDisabled("Compile-time Parameters:");
        
        std::string currentGroup = "";
        bool inGroup = false;

        for (const auto& sl : shader->sliders) {
             // Check if group changed
            if (sl.group != currentGroup) {
                if (inGroup) {
                    ImGui::Unindent();
                    inGroup = false;
                }
                
                currentGroup = sl.group;
                
                if (!currentGroup.empty()) {
                    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", currentGroup.c_str());
                    ImGui::Indent();
                    inGroup = true;
                }
            }

            std::string label = sl.label.empty() ? sl.name : sl.label;
            int value = m_shaderManager->getSliderState(sl.name);
            
            if (ImGui::SliderInt(label.c_str(), &value, sl.min, sl.max)) {
                m_shaderManager->setSliderState(sl.name, value);
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
        
        if (inGroup) {
             ImGui::Unindent();
        }
    }

    if (!shader->switchFlags.empty()) {
        if (hasUniforms || !shader->sliders.empty()) ImGui::Separator();
        
        ImGui::TextDisabled("Feature Toggles:");
        
        std::string currentGroup = "";
        bool inGroup = false;

        for (const auto& sw : shader->switchFlags) {
            
             // Check if group changed
            if (sw.group != currentGroup) {
                if (inGroup) {
                    ImGui::Unindent();
                    inGroup = false;
                }
                
                currentGroup = sw.group;
                
                if (!currentGroup.empty()) {
                    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", currentGroup.c_str());
                    ImGui::Indent();
                    inGroup = true;
                }
            }

            bool enabled = m_shaderManager->getSwitchState(sw.name);
            std::string label = sw.label.empty() ? sw.name : sw.label;
            if (enabled && !sw.labelOn.empty()) {
                label = sw.labelOn;
            }

            if (ImGui::Checkbox(label.c_str(), &enabled)) {
                m_shaderManager->setSwitchState(sw.name, enabled);
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
        
        if (inGroup) {
             ImGui::Unindent();
        }
    }
}

void ParameterPanel::setProject(std::shared_ptr<ShaderProject> shaderProject) {
    m_shaderProject = shaderProject;
}
