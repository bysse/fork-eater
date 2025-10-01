#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform vec3  iResolution;
uniform float iTime;

// Distance function for a sphere
float sdSphere(vec3 p, float s) {
    return length(p) - s;
}

// Scene distance function
float map(vec3 p) {
    return sdSphere(p, 1.0);
}

// Calculate normals
vec3 calcNormal(vec3 p) {
    vec2 e = vec2(0.001, 0.0);
    return normalize(vec3(
        map(p + e.xyy) - map(p - e.xyy),
        map(p + e.yxy) - map(p - e.yxy),
        map(p + e.yyx) - map(p - e.yyx)
    ));
}

void main() {
    vec2 uv = (2.0 * gl_FragCoord.xy - iResolution.xy) / iResolution.y;

    // Camera
    vec3 ro = vec3(0.0, 0.0, 3.0);
    vec3 rd = normalize(vec3(uv, -1.0));

    // Raymarching
    float t = 0.0;
    for (int i = 0; i < 100; i++) {
        vec3 p = ro + rd * t;
        float d = map(p);
        if (d < 0.001) {
            break;
        }
        t += d;
        if (t > 100.0) {
            break;
        }
    }

    vec3 col = vec3(0.0);
    if (t < 100.0) {
        vec3 p = ro + rd * t;
        vec3 n = calcNormal(p);
        vec3 light = normalize(vec3(1.0, 1.0, 2.0));
        float diff = max(dot(n, light), 0.0);
        col = vec3(diff);
    }

    FragColor = vec4(col, 1.0);
}
