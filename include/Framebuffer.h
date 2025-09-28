#pragma once

class Framebuffer {
public:
    Framebuffer(int width, int height);
    ~Framebuffer();

    void bind();
    void unbind();

    unsigned int getTextureId() const { return m_texture; }

private:
    unsigned int m_fbo;
    unsigned int m_texture;
    unsigned int m_rbo;
    int m_width;
    int m_height;
};
