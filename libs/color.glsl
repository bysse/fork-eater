function gamma(vec3 color) {
    return pow( min(color, 1.), vec3(0.44) );
}