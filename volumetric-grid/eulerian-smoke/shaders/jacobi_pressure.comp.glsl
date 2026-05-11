#version 450
#extension GL_GOOGLE_include_directive : require

// One Jacobi iteration of the pressure-Poisson equation:
//   p_new = (p_xp + p_xm + p_yp + p_ym + p_zp + p_zm - h^2 * div) / 6
// Standard 7-point Laplacian inverse on a uniform grid.
//
// Boundary condition for pressure: dp/dn = 0 (Neumann), encoded via sampler
// addressing mode CLAMP_TO_EDGE on u_pressure_old.

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(set = 0, binding = 0) uniform sampler3D u_pressure_old;
layout(set = 0, binding = 1) uniform sampler3D u_divergence;
layout(set = 0, binding = 2, r32f) uniform writeonly image3D u_pressure_new;

layout(set = 0, binding = 3) uniform JacobiUniforms {
    uint  gridSize;
    uint  _pad0;
    uint  _pad1;
    uint  _pad2;
} ju;

void main() {
    ivec3 coord = ivec3(gl_GlobalInvocationID);
    if (any(greaterThanEqual(coord, ivec3(ju.gridSize)))) {
        return;
    }

    float invG = 1.0 / float(ju.gridSize);
    float h = invG;
    vec3 pos = (vec3(coord) + 0.5) * invG;
    vec3 dx = vec3(invG, 0.0, 0.0);
    vec3 dy = vec3(0.0, invG, 0.0);
    vec3 dz = vec3(0.0, 0.0, invG);

    float p_xp = texture(u_pressure_old, pos + dx).r;
    float p_xm = texture(u_pressure_old, pos - dx).r;
    float p_yp = texture(u_pressure_old, pos + dy).r;
    float p_ym = texture(u_pressure_old, pos - dy).r;
    float p_zp = texture(u_pressure_old, pos + dz).r;
    float p_zm = texture(u_pressure_old, pos - dz).r;

    float div = texture(u_divergence, pos).r;

    float p_new = (p_xp + p_xm + p_yp + p_ym + p_zp + p_zm - h * h * div) / 6.0;

    imageStore(u_pressure_new, coord, vec4(p_new, 0.0, 0.0, 0.0));
}
