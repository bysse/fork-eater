#pragma once

#include "glad.h"

class Framebuffer {
public:
    Framebuffer(int width, int height);
    ~Framebuffer();

    void bind();
    void unbind();

    GLuint getTextureId() const;
    int getWidth() const;
    int getHeight() const;
    void resize(int width, int height);

private:
    GLuint m_fbo;
    GLuint m_textureId;
    GLuint m_rbo;
    int m_width;
    int m_height;

    void setupFramebuffer();
    void cleanup();
};
