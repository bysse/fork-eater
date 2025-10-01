#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform float iTime;
uniform vec3 iResolution;

void main()
{
    vec2 uv = TexCoord;
    
    // Create animated rainbow colors
    vec3 col = 0.3 + 0.7 * cos(iTime * 2.0 + uv.xyx + vec3(0, 2, 4));
    
    // Add some movement
    float wave = sin(uv.x * 10.0 + iTime) * sin(uv.y * 10.0 + iTime * 0.5);
    col += 0.2 * wave;
    
    FragColor = vec4(col, 1.0);
}