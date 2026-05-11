#version 450
#extension GL_GOOGLE_include_directive : require

// Divergence of velocity: div = grad . v.
//   div = (vx_xp - vx_xm + vy_yp - vy_ym + vz_zp - vz_zm) / (2 * h)
// where h = 1/gridSize in normalized coords.
//
// The result feeds the Jacobi pressure solver. Divergence is single-buffered
// (write-then-immediately-consumed; no ping-pong needed).

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(set = 0, binding = 0) uniform sampler3D u_velocity;
layout(set = 0, binding = 1, r32f) uniform writeonly image3D u_divergence;

layout(set = 0, binding = 2) uniform DivergenceUniforms {
    uint  gridSize;
    uint  _pad0;
    uint  _pad1;
    uint  _pad2;
} du;

vec3 sampleVel(vec3 pos) {
    return texture(u_velocity, pos).xyz;
}

void main() {
    ivec3 coord = ivec3(gl_GlobalInvocationID);
    if (any(greaterThanEqual(coord, ivec3(du.gridSize)))) {
        return;
    }

    float invG = 1.0 / float(du.gridSize);
    float h = invG;
    vec3 pos = (vec3(coord) + 0.5) * invG;
    vec3 dx = vec3(invG, 0.0, 0.0);
    vec3 dy = vec3(0.0, invG, 0.0);
    vec3 dz = vec3(0.0, 0.0, invG);

    float vx_xp = sampleVel(pos + dx).x;
    float vx_xm = sampleVel(pos - dx).x;
    float vy_yp = sampleVel(pos + dy).y;
    float vy_ym = sampleVel(pos - dy).y;
    float vz_zp = sampleVel(pos + dz).z;
    float vz_zm = sampleVel(pos - dz).z;

    float div = ((vx_xp - vx_xm) + (vy_yp - vy_ym) + (vz_zp - vz_zm)) / (2.0 * h);

    imageStore(u_divergence, coord, vec4(div, 0.0, 0.0, 0.0));
}
