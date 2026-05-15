#version 460
#extension GL_GOOGLE_include_directive : require

// Macroscopic moment reconstruction:
//   rho       = sum_i f_i
//   rho * u   = sum_i f_i * c_i
//   u         = (rho*u) / rho     (with eps guard at solid cells where rho=0)
//
// Runs once per substep, after stream + boundaries; writes the rho and
// velocity 3D images that feed the next substep's collide pass and the
// per-frame raymarch + streamline-advect consumers.
//
// Workgroup: 8x8x4. No subgroup ops needed (each cell's reduction is a
// trivial 19-element sum across thread-local registers).

#include "lattice_constants.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 4) in;

layout(set = 0, binding = 0)          uniform sampler3D f_in_nonrest[18];
layout(set = 0, binding = 1)          uniform sampler3D f_in_rest;
layout(set = 0, binding = 2, r32f)    uniform image3D   rho_out;
layout(set = 0, binding = 3, rgba16f) uniform image3D   velocity_out;

layout(set = 0, binding = 4) uniform MomentsUniforms {
    ivec4 dims;
} U;

void main() {
    ivec3 cell = ivec3(gl_GlobalInvocationID.xyz);
    if (any(greaterThanEqual(cell, U.dims.xyz))) return;

    // Cell-centered normalized coordinate. NEAREST sampler returns f at this cell.
    vec3 ctr = (vec3(cell) + 0.5) / vec3(U.dims.xyz);

    float rho   = texture(f_in_rest, ctr).r;     // i = 0
    vec3  rho_u = vec3(0.0);

    for (int i = 1; i < NUM_DIRS; ++i) {
        float f = texture(f_in_nonrest[i - 1], ctr).r;
        rho   += f;
        rho_u += f * vec3(C_I[i]);
    }

    // Guard against divide-by-zero at solid cells (rho == 0 there after the
    // boundaries pass zeros their populations).
    vec3 u = (rho > 1e-12) ? (rho_u / rho) : vec3(0.0);

    imageStore(rho_out,      cell, vec4(rho, 0.0, 0.0, 0.0));
    imageStore(velocity_out, cell, vec4(u, 0.0));
}
