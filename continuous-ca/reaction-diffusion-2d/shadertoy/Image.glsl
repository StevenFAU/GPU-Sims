// Reaction-Diffusion 2D — Image pass (visualization).
//
// Reads BufA's v channel and runs it through a simple magma colormap.
// iChannel0 = BufferA. Set its filter = Linear, wrap = Clamp.

// Inigo Quilez's 7-coefficient magma fit (public-domain, from Shadertoy).
vec3 magma(float t) {
    const vec3 c0 = vec3(-0.002136, -0.000750, -0.005386);
    const vec3 c1 = vec3( 0.251661,  0.677523,  2.494027);
    const vec3 c2 = vec3( 8.353717, -3.577720,  0.314468);
    const vec3 c3 = vec3(-27.66873, 14.26473, -13.64921);
    const vec3 c4 = vec3( 52.17614, -27.94361, 12.94417);
    const vec3 c5 = vec3(-50.76853, 29.04658,   4.23415);
    const vec3 c6 = vec3( 18.65571, -11.48977, -5.601962);
    return c0 + t * (c1 + t * (c2 + t * (c3 + t * (c4 + t * (c5 + t * c6)))));
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    vec2 c  = texture(iChannel0, uv).xy;
    float v = clamp(c.y, 0.0, 1.0);
    fragColor = vec4(magma(v), 1.0);
}
