#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform float u_time;
uniform vec2 u_resolution;

void main()
{
    vec2 uv = gl_FragCoord.xy / u_resolution.xy;
    
    // Simple time-based color
    vec3 color = vec3(
        0.5 + 0.5 * sin(u_time),
        0.5 + 0.5 * sin(u_time + 2.0),
        0.5 + 0.5 * sin(u_time + 4.0)
    );
    
    FragColor = vec4(color, 1.0);
}