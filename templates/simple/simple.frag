#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform float iTime;
uniform vec3 iResolution;

void main()
{
    vec2 uv = gl_FragCoord.xy / iResolution.xy;
    
    // Simple time-based color
    vec3 color = vec3(
        0.5 + 0.5 * sin(iTime),
        0.5 + 0.5 * sin(iTime + 2.0),
        0.5 + 0.5 * sin(iTime + 4.0)
    );
    
    FragColor = vec4(color, 1.0);
}