#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

#pragma switch(USE_RED, true, "Red: OFF", "Red: ON")
#pragma switch(USE_GREEN, "Green: OFF", "Green: ON")
#pragma switch(USE_BLUE)

#pragma group("Advanced Settings")
#pragma slider(RED_INTENSITY_PCT, 0, 100, "Red Intensity %")
#pragma label(u_density, "Atmospheric Density")
#pragma range(u_density, 0.0, 1.0)
uniform float u_density;
#pragma endgroup

#pragma range(1, 50, "Iteration Count")
uniform float u_iterations;

#pragma range(u_named, 0.0, 10.0, "Named Range")
uniform float u_named;

#pragma range(0.0, 1.0)
uniform float u_no_label;

void main() {
    float r = 0.0;
    float g = 0.0;
    float b = 0.5;
    
#ifdef USE_RED
    r = (u_iterations / 50.0) * (float(RED_INTENSITY_PCT) / 100.0);
#endif

#ifdef USE_GREEN
    g = (u_named / 10.0) * u_density;
#endif

#ifdef USE_BLUE
    b = u_no_label;
#endif

    FragColor = vec4(r, g, b, 1.0);
}