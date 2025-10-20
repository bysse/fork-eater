#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform float iTime;
uniform vec3 iResolution;

void main()
{
    vec2 uv = 0.01*iResolution.xy;
    uv.y = 1.0 - uv.y; // Flip y-coordinate to match TexCoord
    vec3 col = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0, 2, 4));
    FragColor = vec4(col, 1.0);
}