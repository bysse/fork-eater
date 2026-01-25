#include "PreviewPanel.h"
#include "ShaderManager.h"
#include "glad.h"
#include "Logger.h"
#include <cmath>
#include <cstdio>

#include "imgui/imgui.h"

PreviewPanel::PreviewPanel(std::shared_ptr<ShaderManager> shaderManager)
    : m_shaderManager(shaderManager)
    , m_aspectMode(AspectMode::Fixed_16_9)
    , m_previewVAO(0)
    , m_previewVBO(0) {
    
    m_resolution[0] = 1920.0f; // Default 16:9 values, will be updated dynamically
    m_resolution[1] = 1080.0f;
}

PreviewPanel::~PreviewPanel() {
    cleanupPreview();
}

bool PreviewPanel::initialize() {
    setupPreviewQuad();
    return true;
}

void PreviewPanel::render(GLuint textureId, float time, float renderScaleFactor, std::pair<float, float> uvScale) {
    renderPreviewPanel(textureId, time, renderScaleFactor, uvScale);
}

void PreviewPanel::renderPreviewPanel(GLuint textureId, float time, float renderScaleFactor, std::pair<float, float> uvScale) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui::Text("Shader Preview");
    
    // Show current aspect ratio
    const char* aspectNames[] = { "Free", "16:9", "4:3", "1:1", "21:9" };
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(%s)", aspectNames[static_cast<int>(m_aspectMode)]);

    // Display render scale factor if not 1.0f
    if (renderScaleFactor < 1.0f) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "(Rendering at %.0f%%)", renderScaleFactor * 100.0f);
    }
    
    ImGui::Separator();
    
    if (textureId != 0) {
        // Calculate preview size based on aspect ratio settings
        ImVec2 availableSize = ImGui::GetContentRegionAvail();
        availableSize.y -= 60; // Leave space for text above
        
        ImVec2 previewSize = calculatePreviewSize(availableSize);
        
        // Calculate centering offset
        ImVec2 offset = ImVec2(
            std::max(0.0f, (availableSize.x - previewSize.x) * 0.5f),
            std::max(0.0f, (availableSize.y - previewSize.y) * 0.5f)
        );
        
        // Center the preview by moving cursor position
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset.x);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset.y);
        
        ImVec2 imageStartPos = ImGui::GetCursorScreenPos();

        // Display the rendered texture
        // uv0 is Top-Left, uv1 is Bottom-Right
        // Texture coordinate system: (0,0) is bottom-left, (1,1) is top-right
        // Our texture has valid data in [0, uvScale.x] x [0, uvScale.y]
        // We want to map:
        // ImGui Top-Left -> Texture Top-Left of valid region -> (0, uvScale.y)
        // ImGui Bottom-Right -> Texture Bottom-Right of valid region -> (uvScale.x, 0)
        ImGui::Image((void*)(intptr_t)textureId, previewSize, ImVec2(0, uvScale.second), ImVec2(uvScale.first, 0));

        // Interaction
        ImGui::SetCursorScreenPos(imageStartPos);
        ImGui::InvisibleButton("##preview_input", previewSize);
        
        if (ImGui::IsItemActive()) {
            ImVec2 mousePos = ImGui::GetMousePos();
            ImVec2 rectMin = ImGui::GetItemRectMin();
            ImVec2 rectSize = ImGui::GetItemRectSize();
            
            float u = (mousePos.x - rectMin.x) / rectSize.x;
            float v = (mousePos.y - rectMin.y) / rectSize.y;
            
            // Invert Y to match expected coordinate system (0.0 at top is standard for this project based on previous implementation)
            // Previous: io.MousePos.y / height. ImGui 0 is top.
            // So we keep v as is (0 at top).
            // Actually, let's double check if we need to clamp.
            u = std::max(0.0f, std::min(1.0f, u));
            v = std::max(0.0f, std::min(1.0f, v));
            
            m_shaderManager->setMousePosition(u, v);
            m_shaderManager->setMouseClickState(true);

            // Calculate relative movement for u_mouse_rel
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            if (rectSize.x > 0 && rectSize.y > 0) {
                // ImGui Y is down, but our UV Y is up (0 at bottom in standard GL, 
                // but here 0 is top of rect in ImGui). 
                // Wait, if I move mouse UP (negative delta.y), I want my coords to go UP.
                // In standard UV (0 at bottom), UP means INCREASING Y.
                // So negative delta.y -> positive change in Y.
                float dx = delta.x / rectSize.x;
                float dy = -delta.y / rectSize.y;
                m_shaderManager->updateIntegratedMouse(dx, dy);
            }

        } else {
            m_shaderManager->setMouseClickState(false);
        }
    } else {
        ImGui::Text("No shader selected");
        ImGui::Text("Select a shader from the file list to preview");
    }
}

ImVec2 PreviewPanel::calculatePreviewSize(ImVec2 availableSize) {
    if (m_aspectMode == AspectMode::Free) {
        return availableSize;
    }
    
    float targetAspect;
    switch (m_aspectMode) {
        case AspectMode::Fixed_16_9:
            targetAspect = 16.0f / 9.0f;
            break;
        case AspectMode::Fixed_4_3:
            targetAspect = 4.0f / 3.0f;
            break;
        case AspectMode::Fixed_1_1:
            targetAspect = 1.0f;
            break;
        case AspectMode::Fixed_21_9:
            targetAspect = 21.0f / 9.0f;
            break;
        default:
            return availableSize;
    }
    
    float availableAspect = availableSize.x / availableSize.y;
    
    ImVec2 previewSize;
    
    if (availableAspect > targetAspect) {
        // Available area is wider than target aspect ratio
        previewSize.y = availableSize.y;
        previewSize.x = previewSize.y * targetAspect;
    } else {
        // Available area is taller than target aspect ratio
        previewSize.x = availableSize.x;
        previewSize.y = previewSize.x / targetAspect;
    }
    
    return previewSize;
}

void PreviewPanel::setupPreviewQuad() {
    // Full-screen quad vertices
    float vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
        
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &m_previewVAO);
    glGenBuffers(1, &m_previewVBO);
    
    glBindVertexArray(m_previewVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_previewVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void PreviewPanel::cleanupPreview() {
    if (m_previewVAO != 0) {
        glDeleteVertexArrays(1, &m_previewVAO);
        m_previewVAO = 0;
    }
    
    if (m_previewVBO != 0) {
        glDeleteBuffers(1, &m_previewVBO);
        m_previewVBO = 0;
    }
}
