#version 330 core

uniform float u_time;
uniform vec2 u_resolution;

out vec4 FragColor;

void main() {
    vec2 uv = gl_FragCoord.xy / u_resolution.xy;
    
    // Music-synchronized colors that work well with 128 BPM
    float beat = u_time * 128.0 / 60.0;  // Convert to beats
    
    // Pulsing colors on the beat
    vec3 color1 = vec3(0.8, 0.2, 0.8);  // Magenta
    vec3 color2 = vec3(0.2, 0.8, 0.8);  // Cyan
    vec3 color3 = vec3(0.8, 0.8, 0.2);  // Yellow
    
    // Beat-synchronized patterns
    float beatPulse = sin(beat * 3.14159) * 0.5 + 0.5;
    float barPulse = sin(beat * 3.14159 / 4.0) * 0.5 + 0.5;
    
    // Create concentric circles that pulse with the beat
    vec2 center = vec2(0.5, 0.5);
    float dist = length(uv - center);
    
    float circles = sin(dist * 20.0 - u_time * 5.0) * beatPulse;
    
    // Mix colors based on position and beat
    vec3 finalColor = mix(color1, color2, sin(uv.x * 3.14159 + beat)) * circles;
    finalColor = mix(finalColor, color3, barPulse * 0.3);
    
    // Add some sparkle
    float sparkle = fract(sin(dot(uv * 100.0, vec2(12.9898, 78.233))) * 43758.5453);
    if (sparkle > 0.98) {
        finalColor += vec3(1.0) * beatPulse;
    }
    
    FragColor = vec4(finalColor, 1.0);
}