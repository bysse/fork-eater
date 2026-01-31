#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform float iTime;
uniform vec3 iResolution;

#pragma include(lib/noise.glsl)


#pragma group("Test Parameters")

#pragma switch(USE_BLUE)
#pragma label(USE_BLUE, "Use blue for x instead of red")
#pragma endgroup

void main()
{
    vec2 uv = TexCoord.xy / iResolution.xy;

    vec3 col = vec3(0.0, uv.y, 0.0);
    #if USE_BLUE
        col.x = uv.x;
    #else
        col.z = uv.x;
    #endif

    FragColor = vec4(col, 1.0);
}