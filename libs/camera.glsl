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


void freeLookCamera4k(inout vec3 camera, vec3 target, vec2 uv, vec2 mouse, inout vec3 rd) {
#ifdef USE_CAMERA
    vec3 ro = camera;
    
    vec3 fwd = normalize(target - camera);           
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), fwd));
    vec3 up    = cross(fwd, right);
    
    vec3 raw_rd = normalize(uv.x * right + uv.y * up + FORK_CAMERA_FOV * fwd);    
    float theta = atan(raw_rd.x, raw_rd.z); // Horizontal angle
    float phi = asin(raw_rd.y);             // Vertical angle
    
    theta += -mouse.x * FORK_CAMERA_MOUSE_SENSITIVITY;
    phi   += -mouse.y * FORK_CAMERA_MOUSE_SENSITIVITY;
    
    phi = clamp(phi, -1.5, 1.5); 
    
    rd = vec3(
        cos(phi) * sin(theta),
        sin(phi),
        cos(phi) * cos(theta)
    );
    rd = normalize(rd);
#endif
}


void orbitalCamera4k(inout vec3 camera, vec3 target, vec2 uv, vec2 mouse, inout vec3 rd) {
#ifdef USE_CAMERA
    // Orbital rotation using spherical coordinates logic
    vec3 p = camera - target;
    float r = length(p);
    
    // Initial angles based on the input camera position
    // pitch (phi) - angle from Y axis (0 to PI)
    // yaw (theta) - angle in XZ plane
    float phi = acos(p.y / r);
    float theta = atan(p.x, p.z);
    
    // Update angles based on mouse
    // Mouse X controls Yaw (theta)
    // Mouse Y controls Pitch (phi)
    theta += -mouse.x * FORK_CAMERA_MOUSE_SENSITIVITY;
    phi   += -mouse.y * FORK_CAMERA_MOUSE_SENSITIVITY;
    
    // Clamp pitch to avoid flipping at poles (0.01 to PI-0.01)
    phi = clamp(phi, 0.01, 3.14159 - 0.01);
    
    // Convert back to Cartesian
    p.x = r * sin(phi) * sin(theta);
    p.y = r * cos(phi);
    p.z = r * sin(phi) * cos(theta);
        
    vec3 ro = target + p;
    camera = ro; // Update camera position output
    
    vec3 fwd = normalize(target - ro);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), fwd));
    vec3 up = cross(fwd, right);
    
    rd = normalize(uv.x * right + uv.y * up + FORK_CAMERA_FOV * fwd);
#endif
}


void camera4k(inout vec3 camera, vec3 target, vec2 uv, vec2 mouse, inout vec3 rd) {
#ifdef USE_CAMERA
    #ifdef USE_ORBITAL_CAMERA
        orbitalCamera4k(camera, target, uv, mouse, rd);
    #else
        freeLookCamera4k(camera, target, uv, mouse, rd);
    #endif
#endif
}
