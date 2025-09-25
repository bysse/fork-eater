#version 330 core

uniform float u_time;
uniform vec2 u_resolution;

out vec4 FragColor;

void main() {
    FragColor = vec4(u_resolution, 0.0, 1.0);  // GREEN - Sample project shader
}
