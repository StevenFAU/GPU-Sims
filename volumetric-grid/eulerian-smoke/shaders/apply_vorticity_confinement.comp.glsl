#version 450
#extension GL_GOOGLE_include_directive : require

// Vorticity confinement (Fedkiw 2001 eq 14):
//   N      = grad(|omega|) / max(|grad(|omega|)|, 1e-5)    unit gradient of curl magnitude
//   f_vc   = epsilon * h * (N x omega)                     confinement force
//   v_new  = v_old + dt * f_vc                              applied in-place
//
// The zero-guard on the unit-gradient denominator is the load-bearing detail
// (bare division produces NaN that propagates and breaks the sim within ~30 frames).

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(set = 0, binding = 0, rgba16f) uniform image3D u_velocity;    // in-place
layout(set = 0, binding = 1) uniform sampler3D u_curl;

layout(set = 0, binding = 2) uniform VorticityUniforms {
    float dt;
    float epsilon;           // confinement strength (the vorticityStrength slider)
    float _pad0;
    float _pad1;
    uint  gridSize;
    uint  _pad2;
    uint  _pad3;
    uint  _pad4;
} vu;

float curlMag(vec3 pos) {
    return length(texture(u_curl, pos).xyz);
}

void main() {
    ivec3 coord = ivec3(gl_GlobalInvocationID);
    if (any(greaterThanEqual(coord, ivec3(vu.gridSize)))) {
        return;
    }

    float invG = 1.0 / float(vu.gridSize);
    vec3 pos = (vec3(coord) + 0.5) * invG;
    vec3 dx = vec3(invG, 0.0, 0.0);
    vec3 dy = vec3(0.0, invG, 0.0);
    vec3 dz = vec3(0.0, 0.0, invG);

    // grad |omega| via central differences.
    vec3 grad_mag = vec3(
        (curlMag(pos + dx) - curlMag(pos - dx)),
        (curlMag(pos + dy) - curlMag(pos - dy)),
        (curlMag(pos + dz) - curlMag(pos - dz))
    );

    // Zero-guarded normalization (the load-bearing detail).
    float gm_len = length(grad_mag);
    vec3 N = grad_mag / max(gm_len, 1e-5);

    vec3 omega = texture(u_curl, pos).xyz;

    // Confinement force: f_vc = epsilon * h * (N x omega).  h = 1 in normalized units.
    vec3 f_vc = vu.epsilon * cross(N, omega);

    vec4 v = imageLoad(u_velocity, coord);
    v.xyz += vu.dt * f_vc;

    imageStore(u_velocity, coord, v);
}
