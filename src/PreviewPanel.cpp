#include "PreviewPanel.h"
#include "ShaderManager.h"
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <cmath>
#include <cstdio>

#include "imgui/imgui.h"

PreviewPanel::PreviewPanel(std::shared_ptr<ShaderManager> shaderManager)
    : m_shaderManager(shaderManager)
    , m_aspectMode(AspectMode::Fixed_16_9)
    , m_previewVAO(0)
    , m_previewVBO(0)
    , m_previewFramebuffer(0)
    , m_previewTexture(0) {
    
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

void PreviewPanel::render(const std::string& selectedShader, float time) {
    renderPreviewPanel(selectedShader, time);
}

void PreviewPanel::renderPreviewPanel(const std::string& selectedShader, float time) {
    ImGui::Text("Shader Preview");
    
    // Show current aspect ratio
    const char* aspectNames[] = { "Free", "16:9", "4:3", "1:1", "21:9" };
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(%s)", aspectNames[static_cast<int>(m_aspectMode)]);
    
    ImGui::Separator();
    
    if (!selectedShader.empty()) {
        ImGui::Text("Current: %s", selectedShader.c_str());
        
        auto shader = m_shaderManager->getShader(selectedShader);
        if (shader && shader->isValid) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "✓ Shader compiled successfully");
            
            // Calculate preview size based on aspect ratio settings
            ImVec2 availableSize = ImGui::GetContentRegionAvail();
            availableSize.y -= 60; // Leave space for text above
            
            ImVec2 previewSize = calculatePreviewSize(availableSize);
            
            // Update uniforms with current preview resolution
            m_shaderManager->useShader(selectedShader);
            m_shaderManager->setUniform("u_time", time);
            m_resolution[0] = previewSize.x;
            m_resolution[1] = previewSize.y;
            m_shaderManager->setUniform("u_resolution", m_resolution, 2);
            
            // Center the preview if it's smaller than available space
            ImVec2 offset = ImVec2((availableSize.x - previewSize.x) * 0.5f, (availableSize.y - previewSize.y) * 0.5f);
            ImVec2 drawOffset = ImVec2(0.0f, 0.0f); // Offset to use for drawing coordinates
            
            if (offset.x > 0) {
                float newPosX = ImGui::GetCursorPosX() + offset.x;
                if (newPosX >= 0) {
                    ImGui::SetCursorPosX(newPosX);
                    drawOffset.x = offset.x;
                }
            }
            if (offset.y > 0) {
                float newPosY = ImGui::GetCursorPosY() + offset.y;
                if (newPosY >= 0) {
                    ImGui::SetCursorPosY(newPosY);
                    drawOffset.y = offset.y;
                }
            }
            
            // Draw preview
            ImGui::Dummy(previewSize);
            
            // Draw a placeholder with proper aspect ratio
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 screen_pos = ImGui::GetCursorScreenPos();
            ImVec2 canvas_pos = ImVec2(screen_pos.x - drawOffset.x, screen_pos.y - previewSize.y - drawOffset.y);
            ImVec2 canvas_size = previewSize;
            
            // Background (dark gray)
            draw_list->AddRectFilled(canvas_pos, 
                                   ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                                   IM_COL32(30, 30, 30, 255));
            
            // Preview border
            draw_list->AddRect(canvas_pos, 
                             ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                             IM_COL32(100, 100, 100, 255));
            
            // Animated content
            float r = 0.5f + 0.5f * sinf(time);
            float g = 0.5f + 0.5f * sinf(time + 2.0f);
            float b = 0.5f + 0.5f * sinf(time + 4.0f);
            
            float margin = 10.0f;
            draw_list->AddRectFilled(ImVec2(canvas_pos.x + margin, canvas_pos.y + margin),
                                   ImVec2(canvas_pos.x + canvas_size.x - margin, canvas_pos.y + canvas_size.y - margin),
                                   IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), 255));
            
            // Resolution text
            char resText[64];
            snprintf(resText, sizeof(resText), "%.0fx%.0f", previewSize.x, previewSize.y);
            draw_list->AddText(ImVec2(canvas_pos.x + 10, canvas_pos.y + 10), 
                             IM_COL32(255, 255, 255, 200), resText);
                             
            // Preview label
            draw_list->AddText(ImVec2(canvas_pos.x + canvas_size.x/2 - 30, canvas_pos.y + canvas_size.y/2 - 10), 
                             IM_COL32(255, 255, 255, 255), "Preview");
        } else if (shader) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "✗ Compilation failed");
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
    
    if (m_previewFramebuffer != 0) {
        glDeleteFramebuffers(1, &m_previewFramebuffer);
        m_previewFramebuffer = 0;
    }
    
    if (m_previewTexture != 0) {
        glDeleteTextures(1, &m_previewTexture);
        m_previewTexture = 0;
    }
}