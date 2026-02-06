#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform float iTime;
uniform vec3 iResolution;

#pragma include(lib/noise.glsl)

#pragma group("Adjustments")
#pragma label(brightness, "Brightness of background")
uniform float brightness;

#pragma range(0.01, 10.0, "Speed of animation")
uniform float speed;
#pragma endgroup

#pragma group("Color Parameters")
#pragma switch(USE_BLUE, "Use blue for x")

#pragma slider(ITERATIONS, 1, 10, "Number of noise iterations")
#pragma endgroup

void main()
{
    vec2 uv = TexCoord.xy;

    vec3 col = vec3(0.0, uv.y, 0.0);
    #ifdef USE_BLUE
        col.x = uv.x;
    #else
        col.z = uv.x;
    #endif

    FragColor = vec4(col*brightness*(1.0 + sin(iTime*speed)), 1.0);
}