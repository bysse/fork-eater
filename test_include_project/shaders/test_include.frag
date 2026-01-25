#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform vec2 u_resolution;
uniform float u_time;

#pragma include(<utils.glsl>)

void main() {
    float c = circle(TexCoord, 0.5);
    FragColor = vec4(vec3(c), 1.0);
}