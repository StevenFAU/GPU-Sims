#version 460
#extension GL_GOOGLE_include_directive : require

// Streaming step (pull semantics):
//   f_out[x][i] = f_in[x - c_i][i]    for i = 1..18
//   f_out[x][0] = f_in[x][0]          (rest direction does not stream)
//
// Pull avoids inter-thread write conflicts that the equivalent push form
// (`f_out[x + c_i] = f_in[x]`) would create at boundary cells. Out-of-bounds
// reads return clamp-to-edge values; the apply_boundaries pass overwrites the
// affected populations on the same substep so any garbage from clamp doesn't
// propagate.
//
// Workgroup: 8x8x4. Sampling uses the bound CLAMP-NEAREST sampler from C++.

#include "lattice_constants.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 4) in;

layout(set = 0, binding = 0)       uniform sampler3D f_in_nonrest[18];
layout(set = 0, binding = 1)       uniform sampler3D f_in_rest;
layout(set = 0, binding = 2, r32f) uniform image3D   f_out_nonrest[18];
layout(set = 0, binding = 3, r32f) uniform image3D   f_out_rest;

layout(set = 0, binding = 4) uniform StreamUniforms {
    ivec4 dims;
} U;

void main() {
    ivec3 cell = ivec3(gl_GlobalInvocationID.xyz);
    if (any(greaterThanEqual(cell, U.dims.xyz))) return;

    vec3 inv_dims = 1.0 / vec3(U.dims.xyz);

    // Rest: copy through, no streaming.
    {
        vec3 ctr_self = (vec3(cell) + 0.5) * inv_dims;
        imageStore(f_out_rest, cell, vec4(texture(f_in_rest, ctr_self).r, 0.0, 0.0, 0.0));
    }

    // Nonrest: pull from the upstream neighbor (cell - c_i) for each direction.
    for (int i = 1; i < NUM_DIRS; ++i) {
        ivec3 src = cell - C_I[i];
        vec3  ctr = (vec3(src) + 0.5) * inv_dims;
        float v   = texture(f_in_nonrest[i - 1], ctr).r;
        imageStore(f_out_nonrest[i - 1], cell, vec4(v, 0.0, 0.0, 0.0));
    }
}
