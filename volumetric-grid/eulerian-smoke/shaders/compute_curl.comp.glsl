#version 450
#extension GL_GOOGLE_include_directive : require

// Curl of velocity: omega = grad x v.
// Central-difference assembly:
//   omega.x = dv.z/dy - dv.y/dz
//   omega.y = dv.x/dz - dv.z/dx
//   omega.z = dv.y/dx - dv.x/dy
// Writes to a single-buffered curl field (rgba16f, consumed immediately by
// the vorticity confinement pass).

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(set = 0, binding = 0) uniform sampler3D u_velocity;
layout(set = 0, binding = 1, rgba16f) uniform writeonly image3D u_curl;

layout(set = 0, binding = 2) uniform CurlUniforms {
    uint  gridSize;
    uint  _pad0;
    uint  _pad1;
    uint  _pad2;
} cu;

vec3 sampleVel(vec3 pos) {
    return texture(u_velocity, pos).xyz;
}

void main() {
    ivec3 coord = ivec3(gl_GlobalInvocationID);
    if (any(greaterThanEqual(coord, ivec3(cu.gridSize)))) {
        return;
    }

    float invG = 1.0 / float(cu.gridSize);
    vec3 pos = (vec3(coord) + 0.5) * invG;
    vec3 dx = vec3(invG, 0.0, 0.0);
    vec3 dy = vec3(0.0, invG, 0.0);
    vec3 dz = vec3(0.0, 0.0, invG);

    vec3 v_xp = sampleVel(pos + dx);
    vec3 v_xm = sampleVel(pos - dx);
    vec3 v_yp = sampleVel(pos + dy);
    vec3 v_ym = sampleVel(pos - dy);
    vec3 v_zp = sampleVel(pos + dz);
    vec3 v_zm = sampleVel(pos - dz);

    // Central differences. Note: the 1/(2*h) factor is implicit — we leave it
    // out because the vorticity-confinement force normalizes by |omega| anyway,
    // and the magnitude scale gets absorbed into the user-tunable epsilon.
    vec3 omega = vec3(
        (v_yp.z - v_ym.z) - (v_zp.y - v_zm.y),    // omega.x
        (v_zp.x - v_zm.x) - (v_xp.z - v_xm.z),    // omega.y
        (v_xp.y - v_xm.y) - (v_yp.x - v_ym.x)     // omega.z
    );

    imageStore(u_curl, coord, vec4(omega, 0.0));
}
