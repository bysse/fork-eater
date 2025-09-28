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

void PreviewPanel::render(GLuint textureId, float time) {
    renderPreviewPanel(textureId, time);
}

void PreviewPanel::renderPreviewPanel(GLuint textureId, float time) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui::Text("Shader Preview");
    
    // Show current aspect ratio
    const char* aspectNames[] = { "Free", "16:9", "4:3", "1:1", "21:9" };
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(%s)", aspectNames[static_cast<int>(m_aspectMode)]);
    
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
        
        // Display the rendered texture
        ImGui::Image((void*)(intptr_t)textureId, previewSize, ImVec2(0, 1), ImVec2(1, 0));
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
