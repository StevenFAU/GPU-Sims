#version 450
#extension GL_GOOGLE_include_directive : require

// Pressure projection step: v_new = v_old - grad(p).
//   grad(p).x = (p_xp - p_xm) / (2 * h)
//   grad(p).y = (p_yp - p_ym) / (2 * h)
//   grad(p).z = (p_zp - p_zm) / (2 * h)
// In-place write on velocity (the cell's velocity write depends only on
// pressure neighbors, not velocity neighbors).

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(set = 0, binding = 0, rgba16f) uniform image3D u_velocity;     // in-place
layout(set = 0, binding = 1) uniform sampler3D u_pressure;

layout(set = 0, binding = 2) uniform ProjectUniforms {
    uint  gridSize;
    uint  _pad0;
    uint  _pad1;
    uint  _pad2;
} pu;

void main() {
    ivec3 coord = ivec3(gl_GlobalInvocationID);
    if (any(greaterThanEqual(coord, ivec3(pu.gridSize)))) {
        return;
    }

    float invG = 1.0 / float(pu.gridSize);
    float h = invG;
    vec3 pos = (vec3(coord) + 0.5) * invG;
    vec3 dx = vec3(invG, 0.0, 0.0);
    vec3 dy = vec3(0.0, invG, 0.0);
    vec3 dz = vec3(0.0, 0.0, invG);

    float p_xp = texture(u_pressure, pos + dx).r;
    float p_xm = texture(u_pressure, pos - dx).r;
    float p_yp = texture(u_pressure, pos + dy).r;
    float p_ym = texture(u_pressure, pos - dy).r;
    float p_zp = texture(u_pressure, pos + dz).r;
    float p_zm = texture(u_pressure, pos - dz).r;

    vec3 grad_p = vec3(p_xp - p_xm, p_yp - p_ym, p_zp - p_zm) / (2.0 * h);

    vec4 v = imageLoad(u_velocity, coord);
    v.xyz -= grad_p;

    imageStore(u_velocity, coord, v);
}
