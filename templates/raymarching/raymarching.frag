#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform float iTime;
uniform vec3 iResolution;

// Distance function for a sphere
float sdSphere(vec3 p, float s) {
    return length(p) - s;
}

// Distance function for a plane
float sdPlane(vec3 p, vec4 n) {
    return dot(p, n.xyz) + n.w;
}

// Scene distance function
float map(vec3 p) {
    float d = sdPlane(p, vec4(0.0, 1.0, 0.0, 1.0));
    d = min(d, sdSphere(p - vec3(0.0, 0.5, 0.0), 0.5));
    return d;
}

// Calculate normal
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
    vec3 ro = vec3(2.0 * cos(iTime), 1.0, 2.0 * sin(iTime));
    vec3 rd = normalize(vec3(uv, -1.0));

    float t = 0.0;
    for (int i = 0; i < 100; i++) {
        vec3 p = ro + rd * t;
        float d = map(p);
        if (d < 0.001) {
            break;
        }
        t += d;
    }

    vec3 col = vec3(0.0);
    float depth = t;

    if (t < 100.0) {
        vec3 p = ro + rd * t;
        vec3 normal = calcNormal(p);
        vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
        float diff = max(dot(normal, lightDir), 0.0);
        col = vec3(diff);
    }

    FragColor = vec4(col, depth);
}