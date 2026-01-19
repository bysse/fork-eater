/*

*/

#pragma group("Camera")
#pragma switch(USE_CAMERA, on)
#pragma label(USE_CAMERA, "Enable")
#pragma switch(USE_ORBITAL_CAMERA)
#pragma label(USE_ORBITAL_CAMERA, "Free look / Orbital")
#pragma endgroup()

#ifdef USE_CAMERA

#define FORK_CAMERA_MOUSE_SENSITIVITY 3.14
#define FORK_CAMERA_FOV 1.5

mat2 fork_rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}
#endif


void freeLookCamera4k(vec3 camera, vec3 target, vec2 uv, vec4 iMouse, inout vec3 rd) {
#ifdef USE_CAMERA
    vec3 ro = camera;
    
    vec3 fwd = normalize(target - camera);           
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), fwd));
    vec3 up    = cross(fwd, right);
    
    vec3 raw_rd = normalize(uv.x * right + uv.y * up + FORK_CAMERA_FOV * fwd);    
    float theta = atan(raw_rd.x, raw_rd.z); // Horizontal angle
    float phi = asin(raw_rd.y);             // Vertical angle
    
    theta += -iMouse.x * FORK_CAMERA_MOUSE_SENSITIVITY;
    phi   += -iMouse.y * FORK_CAMERA_MOUSE_SENSITIVITY;
    
    phi = clamp(phi, -1.5, 1.5); 
    
    rd = vec3(
        cos(phi) * sin(theta),
        sin(phi),
        cos(phi) * cos(theta)
    );
    rd = normalize(rd);
#endif
}


void orbitalCamera4k(vec3 camera, vec3 target, vec2 uv, vec4 iMouse, inout vec3 rd) {
#ifdef USE_CAMERA
    vec3 p = camera - target;
    p.xz *= fork_rot(-iMouse.x * FORK_CAMERA_MOUSE_SENSITIVITY);
    p.yz *= fork_rot(-iMouse.y * FORK_CAMERA_MOUSE_SENSITIVITY);
        
    vec3 ro = target + p;    
    vec3 fwd = normalize(target - ro);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), fwd));
    vec3 up = cross(fwd, right);
    
    rd = normalize(uv.x * right + uv.y * up + FORK_CAMERA_FOV * fwd);
#endif
}


void camera4k(vec3 camera, vec3 target, vec2 uv, vec4 iMouse, inout vec3 rd) {
#ifdef USE_CAMERA
    #ifdef USE_ORBITAL_CAMERA
        orbitalCamera4k(camera, target, uv, iMouse, rd);
    #else
        freeLookCamera4k(camera, target, uv, iMouse, rd);
    #endif
#endif
}
