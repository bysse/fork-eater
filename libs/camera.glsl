/*

*/

#pragma group("Camera")
#pragma switch(USE_CAMERA, true, "Disable", "Enable")
#pragma switch(USE_ORBITAL_CAMERA, true, "Free look", "Orbital")
#pragma endgroup()

#ifndef FORK_CAMERA_FOV
#define FORK_CAMERA_FOV 1.5
#endif

#ifdef USE_CAMERA
uniform vec2 u_fork_cam_mouse;
#define FORK_CAMERA_MOUSE_SENSITIVITY 3.14

mat2 fork_rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

void freeLookCamera4k(vec3 ro, vec3 ta, vec2 uv, out vec3 rd) {
    vec2 m = u_fork_cam_mouse;
    vec3 fwd = normalize(ta - ro);           
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), fwd));
    vec3 up    = cross(fwd, right);
    vec3 raw_rd = normalize(uv.x * right + uv.y * up + FORK_CAMERA_FOV * fwd);    
    float theta = atan(raw_rd.x, raw_rd.z) - m.x * FORK_CAMERA_MOUSE_SENSITIVITY;
    float phi   = clamp(asin(raw_rd.y) - m.y * FORK_CAMERA_MOUSE_SENSITIVITY, -1.5, 1.5); 
    rd = normalize(vec3(cos(phi) * sin(theta), sin(phi), cos(phi) * cos(theta)));
}

void orbitalCamera4k(inout vec3 ro, vec3 ta, vec2 uv, out vec3 rd) {
    vec2 m = u_fork_cam_mouse;
    vec3 p = ro - ta;
    float r = length(p);
    float phi = clamp(acos(p.y / r) - m.y * FORK_CAMERA_MOUSE_SENSITIVITY, 0.01, 3.1415);
    float theta = atan(p.x, p.z) - m.x * FORK_CAMERA_MOUSE_SENSITIVITY;
    ro = ta + vec3(r * sin(phi) * sin(theta), r * cos(phi), r * sin(phi) * cos(theta));
    vec3 fwd = normalize(ta - ro);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), fwd));
    rd = normalize(uv.x * right + uv.y * cross(fwd, right) + FORK_CAMERA_FOV * fwd);
}

#endif

#ifdef USE_CAMERA

void editorCamera(inout vec3 ro, vec3 ta, vec2 uv, out vec3 rd) {
    #ifdef USE_CAMERA    
    #ifdef USE_ORBITAL_CAMERA
        orbitalCamera4k(ro, ta, uv, rd);
    #else
        freeLookCamera4k(ro, ta, uv, rd);
    #endif
    #endif
}

void demoCamera(vec3 ro, vec3 ta, vec2 uv, vec3 rd) {
}

#else

void editorCamera(vec3 ro, vec3 ta, vec2 uv, vec3 rd) {
}

void demoCamera(vec3 ro, vec3 ta, vec2 uv, out vec3 rd) {
    vec3 f = normalize(ta - ro);           
    vec3 r = normalize(cross(abs(f.y) > 0.99 ? vec3(0,0,1) : vec3(0,1,0), f));
    rd = normalize(uv.x * r + uv.y * cross(f, r) + FORK_CAMERA_FOV * f);
}
#endif
