
vec3 intersect(vec3 ro, vec3 rd) {
    float t = 0.0;
    float prev_d = 0.0;
    float safety = 1.0;

    for(int i = 0; i < 100; i++) {
        float d = map(ro + rd * t);
        
        // Estimate local Lipschitz if we've moved at least once
        if (t > 0.0) {
            float delta_p = d_prev - d; // How much the SDF actually changed
            float step_size = last_step; // How much we moved
            // L_local = delta_SDF / delta_P
            // We ensure it's at least 1.0 to avoid division by zero or overstepping
            float L_est = max(1.0, abs(delta_p) / step_size);
            safety = 1.0 / L_est;
        }

        if(d < 0.001 || t > max_dist) break;
        
        float actual_step = d * safety;
        t += actual_step;
        
        // Store values for next iteration
        d_prev = d;
        last_step = actual_step;
    }
}