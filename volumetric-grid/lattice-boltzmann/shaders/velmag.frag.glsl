#version 460

// Volume raymarch of velocity magnitude with colormap LUT.
//
// Generalized from eulerian-smoke's raymarch.frag.glsl for non-unit-cube
// domains via a uniform-driven `volumeMin/volumeMax` AABB. The shading
// model is transfer-function-driven (|u| -> LUT) rather than ES's
// physically-driven (single-scatter absorption + emission + Beer-Lambert):
//   - For each ray-step, sample velocity field, compute |u|, normalize
//     into [velmagMin, velmagMax], LUT-fetch a color.
//   - Accumulate (color * absorption_step) with alpha-blended transmittance.
//
// The full ES raymarch is NOT promoted here — see Decision 8 in the
// Phase 12 spec. Promotion gate is consumer #3 of the volume-raymarch
// pattern (rule of three).

layout(location = 0) in  vec2 v_uv;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler3D velocity_field;     // rgba16f
layout(set = 0, binding = 1) uniform sampler2D colormap_lut;       // RGBA strip
layout(set = 0, binding = 2) uniform sampler2D blue_noise;         // jitter LUT

layout(set = 0, binding = 3) uniform RaymarchUniforms {
    mat4  invViewProj;
    vec4  cameraPos;
    vec4  volumeMin;
    vec4  volumeMax;
    vec4  volumeAspect;     // (Nx, Ny, Nz, max_dim) — reserved for shadow gen.
    int   raymarchSteps;
    int   _pad0;
    float velmagAbsorption;
    float velmagMin;
    float velmagMax;
    float exposure;
    float _pad1;
    float _pad2;
} U;

vec3 reconstructRayDir(vec2 uv) {
    vec4 ndc_far  = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    vec4 world_far = U.invViewProj * ndc_far;
    world_far /= world_far.w;
    return normalize(world_far.xyz - U.cameraPos.xyz);
}

vec2 slabIntersect(vec3 ro, vec3 rd, vec3 bmin, vec3 bmax) {
    vec3 inv = 1.0 / rd;
    vec3 t0  = (bmin - ro) * inv;
    vec3 t1  = (bmax - ro) * inv;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float t_enter = max(max(tmin.x, tmin.y), tmin.z);
    float t_exit  = min(min(tmax.x, tmax.y), tmax.z);
    return vec2(t_enter, t_exit);
}

void main() {
    vec3 ro = U.cameraPos.xyz;
    vec3 rd = reconstructRayDir(v_uv);

    vec2 t = slabIntersect(ro, rd, U.volumeMin.xyz, U.volumeMax.xyz);
    if (t.y < t.x || t.y < 0.0) {
        out_color = vec4(0.0);
        return;
    }
    float t0 = max(t.x, 0.0);
    float t1 = t.y;
    float ray_len = t1 - t0;

    float jitter = texture(blue_noise, gl_FragCoord.xy / 256.0).r;
    float dt     = ray_len / float(U.raymarchSteps);
    float t_cur  = t0 + jitter * dt;

    vec3  color = vec3(0.0);
    float trans = 1.0;
    vec3  domain_extent = U.volumeMax.xyz - U.volumeMin.xyz;
    float vmag_range    = max(U.velmagMax - U.velmagMin, 1e-6);

    for (int i = 0; i < U.raymarchSteps && trans > 0.01; ++i) {
        vec3 p   = ro + rd * t_cur;
        vec3 uvw = (p - U.volumeMin.xyz) / domain_extent;
        if (all(greaterThanEqual(uvw, vec3(0.0))) && all(lessThanEqual(uvw, vec3(1.0)))) {
            vec3  u    = texture(velocity_field, uvw).xyz;
            float mag  = length(u);
            float t01  = clamp((mag - U.velmagMin) / vmag_range, 0.0, 1.0);
            vec3  c    = texture(colormap_lut, vec2(t01, 0.5)).rgb;
            float abs_step = U.velmagAbsorption * dt * t01;
            color += trans * c * abs_step * U.exposure;
            trans *= exp(-abs_step);
        }
        t_cur += dt;
    }

    out_color = vec4(color, 1.0 - trans);
}
