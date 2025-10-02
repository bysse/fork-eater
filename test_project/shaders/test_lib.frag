#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform vec3  iResolution;
uniform float iTime;

#pragma include("<utils.glsl>")

void main() {
    vec2 uv = gl_FragCoord.xy / iResolution.xy;
    float c = circle(uv, 0.5);
    FragColor = vec4(c, c, c, 1.0);
}
