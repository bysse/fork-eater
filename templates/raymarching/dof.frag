#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform float iTime;
uniform vec3 iResolution;
uniform sampler2D iChannel0;

float getBlur(float depth) {
    return clamp(abs(depth - 5.0) * 0.1, 0.0, 1.0);
}

void main() {
    vec2 uv = gl_FragCoord.xy / iResolution.xy;
    vec4 color = texture(iChannel0, uv);
    float depth = color.a;

    float blur = getBlur(depth);

    vec3 blurred_color = vec3(0.0);
    if (blur > 0.01) {
        for (int x = -4; x <= 4; x++) {
            for (int y = -4; y <= 4; y++) {
                vec2 offset = vec2(float(x), float(y)) * blur / iResolution.xy;
                blurred_color += texture(iChannel0, uv + offset).rgb;
            }
        }
        blurred_color /= 81.0;
    } else {
        blurred_color = color.rgb;
    }

    FragColor = vec4(blurred_color, 1.0);
}
