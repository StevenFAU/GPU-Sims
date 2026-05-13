// initial_fill.comp.glsl — Brick + optional droplet preset distribution.
// Called at preset-apply and on tier-change.
#version 460

layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict writeonly buffer Particles {
    vec4 p[];
};

layout(set=0, binding=1, std140) uniform U {
    vec4 brickMin_radius;         // .xyz = brick min; .w = particle radius
    vec4 brickMax_pad;
    vec4 dropletCenter_radius;
    vec4 dropletVel_addFlag;      // .xyz = velocity; .w = nonzero if droplet enabled
    uint brickParticleCount;
    uint totalInitialParticles;
    uint _pad0;
    uint _pad1;
};

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= totalInitialParticles) return;

    float radius  = brickMin_radius.w;
    float spacing = 2.0 * radius;
    vec3  bmin    = brickMin_radius.xyz;
    vec3  bmax    = brickMax_pad.xyz;
    vec3  extent  = bmax - bmin;
    uvec3 dims    = uvec3(max(extent / spacing, vec3(1.0)));

    vec3 pos;
    vec3 vel   = vec3(0.0);
    uint flags = 0u;

    if (gid < brickParticleCount) {
        uint i = gid % max(dims.x, 1u);
        uint j = (gid / max(dims.x, 1u)) % max(dims.y, 1u);
        uint k = gid / max(dims.x * dims.y, 1u);
        pos = bmin + (vec3(float(i), float(j), float(k)) + vec3(0.5)) * spacing;
    } else {
        uint  dgid    = gid - brickParticleCount;
        float dradius = dropletCenter_radius.w;
        uvec3 ddims   = uvec3(max(vec3(2.0 * dradius / spacing), vec3(1.0)));
        uint  i       = dgid % max(ddims.x, 1u);
        uint  j       = (dgid / max(ddims.x, 1u)) % max(ddims.y, 1u);
        uint  k       = dgid / max(ddims.x * ddims.y, 1u);
        vec3  local   = (vec3(float(i), float(j), float(k))
                        - vec3(ddims) * 0.5 + vec3(0.5)) * spacing;
        float ll      = length(local);
        if (ll > dradius) local = (local / max(ll, 1e-7)) * dradius * 0.95;
        pos   = dropletCenter_radius.xyz + local;
        vel   = dropletVel_addFlag.xyz;
        flags = 2u;
    }

    p[gid * 8u + 0u] = vec4(pos, radius);
    p[gid * 8u + 1u] = vec4(vel, 0.0);
    p[gid * 8u + 2u] = vec4(0.0);
    p[gid * 8u + 3u] = vec4(0.0);
    p[gid * 8u + 5u] = uintBitsToFloat(uvec4(gid, 0u, flags, 0u));
    p[gid * 8u + 6u] = vec4(0.0);
    p[gid * 8u + 7u] = vec4(0.0);
}
