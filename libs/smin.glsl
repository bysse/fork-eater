vec2 smin(vec2 res1, vec2 res2, float k) {
    float h = clamp(0.5 + 0.5 * (res2.x - res1.x) / k, 0.0, 1.0);        
    return vec2(
        mix(res2.x, res1.x, h) - k * h * (1.0 - h), 
        mix(res2.y, res1.y, h)
    );
}

float smin( float a, float b, float k )
{
    k *= 4.0;
    float h = max( k-abs(a-b), 0.0 )/k;
    return min(a,b) - h*h*k*(1.0/4.0);
}