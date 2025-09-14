#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform float u_time;
uniform vec2 u_resolution;

void main()
{
    vec2 uv = TexCoord;
    
    // Create animated rainbow colors (modified for testing)
    vec3 col = 0.3 + 0.7 * cos(u_time * 2.0 + uv.xyx + vec3(0, 2, 4));
    
    // Add some movement
    float wave = sin(uv.x * 10.0 + u_time) * sin(uv.y * 10.0 + u_time * 0.5);
    col += 1.2 * wave;
    col *= 0.0;
    
    FragColor = vec4(col, 1.0);
}