#version 450
#extension GL_GOOGLE_include_directive : require

// Gray-Scott reaction-diffusion update kernel.
// One workgroup_size invocation per cell, dispatched (GRID/8)³ workgroups.
// Reads u_curr / v_curr (sampled with REPEAT for periodic BCs, NEAREST filter
// so finite differences see exact neighbor values), writes u_next / v_next.

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

// Sampler-bound 3D textures for read (NEAREST + REPEAT)
layout(set = 0, binding = 0) uniform sampler3D u_curr;
layout(set = 0, binding = 1) uniform sampler3D v_curr;

// Storage 3D images for write (no sampler)
layout(set = 0, binding = 2, r32f) uniform writeonly image3D u_next;
layout(set = 0, binding = 3, r32f) uniform writeonly image3D v_next;

layout(set = 0, binding = 4) uniform RdUniforms {
    float Du;
    float Dv;
    float F;
    float k;
    float dt;
    uint  gridSize;
    uint  _pad0;
    uint  _pad1;
} rd;

void main() {
    ivec3 coord = ivec3(gl_GlobalInvocationID);
    if (any(greaterThanEqual(coord, ivec3(rd.gridSize)))) {
        return;
    }

    // Sample center + 6 face neighbors via REPEAT-addressed sampler.
    // Use a normalized texel offset; the sampler handles wrap.
    float invG = 1.0 / float(rd.gridSize);
    vec3 uvw_c = (vec3(coord) + 0.5) * invG;

    vec3 dx = vec3(invG, 0.0, 0.0);
    vec3 dy = vec3(0.0, invG, 0.0);
    vec3 dz = vec3(0.0, 0.0, invG);

    float u_c = texture(u_curr, uvw_c).r;
    float u_xp = texture(u_curr, uvw_c + dx).r;
    float u_xm = texture(u_curr, uvw_c - dx).r;
    float u_yp = texture(u_curr, uvw_c + dy).r;
    float u_ym = texture(u_curr, uvw_c - dy).r;
    float u_zp = texture(u_curr, uvw_c + dz).r;
    float u_zm = texture(u_curr, uvw_c - dz).r;

    float v_c = texture(v_curr, uvw_c).r;
    float v_xp = texture(v_curr, uvw_c + dx).r;
    float v_xm = texture(v_curr, uvw_c - dx).r;
    float v_yp = texture(v_curr, uvw_c + dy).r;
    float v_ym = texture(v_curr, uvw_c - dy).r;
    float v_zp = texture(v_curr, uvw_c + dz).r;
    float v_zm = texture(v_curr, uvw_c - dz).r;

    // Laplacian via 6-point stencil with normalized dx = 1.
    float lap_u = u_xp + u_xm + u_yp + u_ym + u_zp + u_zm - 6.0 * u_c;
    float lap_v = v_xp + v_xm + v_yp + v_ym + v_zp + v_zm - 6.0 * v_c;

    // Gray-Scott reaction.
    float uvv = u_c * v_c * v_c;

    float u_new = u_c + rd.dt * (rd.Du * lap_u - uvv + rd.F * (1.0 - u_c));
    float v_new = v_c + rd.dt * (rd.Dv * lap_v + uvv - (rd.F + rd.k) * v_c);

    // Clamp to a sane range to keep numerical noise from sending fields negative
    // or unbounded under aggressive parameter combinations. Standard Pearson
    // regions stay well within [0, 1.5]; this is a guard for slider extremes.
    u_new = clamp(u_new, 0.0, 2.0);
    v_new = clamp(v_new, 0.0, 2.0);

    imageStore(u_next, coord, vec4(u_new, 0.0, 0.0, 0.0));
    imageStore(v_next, coord, vec4(v_new, 0.0, 0.0, 0.0));
}
