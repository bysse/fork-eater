#pragma group("Raymarching vec3")
#pragma slider(RAYMARCH_STEPS, 25, 200, "Steps")
#pragma slider(RAYMARCH_MAX_DISTANCE, 5, 1000, "Max Dist")
#pragma switch(RAYMARCH_RELAXED, false, "Normal tracing", "Relaxed tracing")
#pragma switch(RAYMARCH_INTERNAL, false, "March positive", "March both")
#pragma endgroup()

#ifndef EPS
vec3 eps = vec3(0.01, 0.0, 0.0);
#define EPS eps
#endif

#ifndef RAYMARCH_MAX_DISTANCE
#define RAYMARCH_MAX_DISTANCE   10000.0
#endif

#ifndef RAYMARCH_MIN_DISTANCE
#define RAYMARCH_MIN_DISTANCE   0.02
#endif


// Return the normalized normal vector at point p by sampling the distance field at p and nearby points.
vec3 normal(vec3 p) {
	float d = map(p).x;
	return normalize(vec3(
		d - map(p-EPS.xyz).x,
		d - map(p-EPS.yxz).x,
		d - map(p-EPS.yzx).x
	));
}

// Marches along a ray and returns a vec4 where:
// x = distance to closest surface
// y = closeness to the surface (negative if inside)
// z = material id (-1.0 for none)
// w = emissive (0.0 for none)
vec4 intersect(vec3 ro, vec3 rd) {
	// .x = t
    // .y = dt
	// .z = material
	// .w = min(t) emissive
	vec4 hit = vec4(0.1, 0.1, 0., 0.);

#ifdef RAYMARCH_RELAXED
#ifdef RAYMARCH_INTERNAL
	float prev = 0.0, sgn = map(ro).x<0.0 ? -1.0 : 1.0;
	for (int i=0; i<RAYMARCH_STEPS; i++ ) { 		
		if (abs(hit.y) > RAYMARCH_MIN_DISTANCE) {
            hit.yzw = map(ro + rd * hit.x);
			float L_est = 1.0 / max(1.0, abs( sgn * hit.y - prev) / (hit.x - prev));
			prev = sgn * hit.y * L_est;
	        hit.x += sgn * hit.y * L_est;
		}
		if (hit.x > RAYMARCH_MAX_DISTANCE) {
			hit.z = -1;
			break;
		}
	}
#else	
	float prev = 0.0;
	for (int i=0; i<RAYMARCH_STEPS; i++ ) { 		
		if (abs(hit.y) > RAYMARCH_MIN_DISTANCE) {
            hit.yzw = map(ro + rd * hit.x);
			float L_est = 1.0 / max(1.0, abs( hit.y - prev) / (hit.x - prev));
			prev = hit.y * L_est;
	        hit.x += hit.y * L_est;
		}
		if (hit.x > RAYMARCH_MAX_DISTANCE) {
			hit.z = -1;
			break;
		}
	}
#endif	
#else

#ifdef RAYMARCH_INTERNAL
	float sgn = map(ro).x<0.0 ? -1.0 : 1.0;
	for (int i=0; i<RAYMARCH_STEPS; i++ ) { 		
	if (abs(hit.y) > RAYMARCH_MIN_DISTANCE) {
		hit.yzw = map(ro + rd * hit.x);
		hit.x += hit.y * sgn;
	}
	if (hit.x > RAYMARCH_MAX_DISTANCE) {
		hit.z = -1;
		break;
	}
}
#else
	for (int i=0; i<RAYMARCH_STEPS; i++ ) { 		
		if (abs(hit.y) > RAYMARCH_MIN_DISTANCE) {
			hit.yzw = map(ro + rd * hit.x);
			hit.x += hit.y;
		}
		if (hit.x > RAYMARCH_MAX_DISTANCE) {
			hit.z = -1;
			break;
		}
	}
#endif		
#endif
    return hit;
}