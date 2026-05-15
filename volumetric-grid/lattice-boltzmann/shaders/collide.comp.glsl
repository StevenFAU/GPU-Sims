#version 460
#extension GL_GOOGLE_include_directive : require

// BGK single-relaxation-time collision:
//   f'_i(x) = (1 - 1/tau) * f_i(x) + (1/tau) * feq_i(rho, u)
//
// Reads (rho, u) from the moment buffers populated by the *previous* substep's
// compute_moments pass (lag-1; init_equilibrium primes them on substep 0). The
// rho/velocity textures are sampled at the cell center via NEAREST.
//
// References:
//   - tools/integrity/docs/algebraic/d3q19.md § 4.1 (Maxwell-Boltzmann eq form)
//   - references/lbm-principles-practice/chapter13/cpu/LBM.cpp:97-181 — D2Q9
//     pattern reference; 3D D3Q19 generalization per d3q19.md § 2.2. The
//     factored form `omtauinv*f + tauinv*w*rho*[1 - 1.5(u.u) + (c.3u)(1+0.5(c.3u))]`
//     is algebraically identical to the canonical form below; banked at v1.1
//     as a perf optimisation (~5% on RDNA via FMA chain).
//
// cat3.upstream-citation: tools/integrity/docs/algebraic/d3q19.md § 4.1
// cat1.upstream-citation: references/lbm-principles-practice/chapter13/cpu/LBM.cpp:97 (D2Q9 pattern)

#include "lattice_constants.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 4) in;

layout(set = 0, binding = 0, r32f) uniform image3D   f_nonrest[18];
layout(set = 0, binding = 1, r32f) uniform image3D   f_rest;
layout(set = 0, binding = 2)       uniform sampler3D rho_in;
layout(set = 0, binding = 3)       uniform sampler3D velocity_in;

layout(set = 0, binding = 4) uniform CollideUniforms {
    ivec4 dims;
    float tau_inv;
    float omtau_inv;
    float _pad0;
    float _pad1;
} U;

void main() {
    ivec3 cell = ivec3(gl_GlobalInvocationID.xyz);
    if (any(greaterThanEqual(cell, U.dims.xyz))) return;

    vec3 ctr = (vec3(cell) + 0.5) / vec3(U.dims.xyz);
    float rho = texture(rho_in,      ctr).r;
    vec3  u   = texture(velocity_in, ctr).xyz;

    // i = 0 (rest direction).
    {
        float f   = imageLoad(f_rest, cell).r;
        float feq = feq_i(0, rho, u);
        float fp  = U.omtau_inv * f + U.tau_inv * feq;
        imageStore(f_rest, cell, vec4(fp, 0.0, 0.0, 0.0));
    }

    // i = 1..18 (nonrest directions).
    for (int i = 1; i < NUM_DIRS; ++i) {
        float f   = imageLoad(f_nonrest[i - 1], cell).r;
        float feq = feq_i(i, rho, u);
        float fp  = U.omtau_inv * f + U.tau_inv * feq;
        imageStore(f_nonrest[i - 1], cell, vec4(fp, 0.0, 0.0, 0.0));
    }
}
