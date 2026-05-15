#version 460
#extension GL_GOOGLE_include_directive : require

// Initializes the LBM distribution functions to equilibrium at every cell:
//   f_i = feq_i(rho=1, u=u_inf)
// Also seeds the moment buffers (rho, velocity) so the next collide pass has
// valid lag-1 moments. Solid cells are NOT special-cased here; the first
// boundaries pass after stream zeros their populations.
//
// Workgroup: 8x8x4 = 256 threads (matches eulerian-smoke pattern, comfortably
// inside the 256-thread workgroup limit). u_inf passed via push constants so
// the same uniform-buffer struct is shared with the per-substep collide pass.

#include "lattice_constants.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 4) in;

layout(set = 0, binding = 0, r32f)    uniform image3D f_nonrest[18];
layout(set = 0, binding = 1, r32f)    uniform image3D f_rest;
layout(set = 0, binding = 2, r32f)    uniform image3D rho_out;
layout(set = 0, binding = 3, rgba16f) uniform image3D velocity_out;

layout(set = 0, binding = 4) uniform InitUniforms {
    ivec4 dims;        // (Nx, Ny, Nz, _)
    float tau_inv;
    float omtau_inv;
    float _pad0;
    float _pad1;
} U;

layout(push_constant) uniform PushConsts {
    vec4 u_inf;        // xyz = free-stream velocity, w = unused (kept for std430 alignment)
} pc;

void main() {
    ivec3 cell = ivec3(gl_GlobalInvocationID.xyz);
    if (any(greaterThanEqual(cell, U.dims.xyz))) return;

    float rho = 1.0;
    vec3  u   = pc.u_inf.xyz;

    imageStore(rho_out,      cell, vec4(rho, 0.0, 0.0, 0.0));
    imageStore(velocity_out, cell, vec4(u, 0.0));
    imageStore(f_rest,       cell, vec4(feq_i(0, rho, u), 0.0, 0.0, 0.0));
    for (int i = 1; i < NUM_DIRS; ++i) {
        imageStore(f_nonrest[i - 1], cell, vec4(feq_i(i, rho, u), 0.0, 0.0, 0.0));
    }
}
