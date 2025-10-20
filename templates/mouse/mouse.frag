#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform vec2 u_resolution;
uniform float u_time;
uniform vec4 u_mouse;

void main()
{
    vec2 uv = TexCoord;
    vec3 color = vec3(uv.x, uv.y, 0.5);

    // Change color based on mouse position
    color.r = u_mouse.x;
    color.g = u_mouse.y;

    // Draw a circle at the mouse position
    float dist = distance(uv, u_mouse.xy);
    if (dist < 0.05) {
        color = vec3(1.0, 1.0, 0.0);
    }

    FragColor = vec4(color, 1.0);
}
