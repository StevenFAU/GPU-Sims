// apply_emitter.comp.glsl — Reserve-tail emitter inject. Each thread spawns one
// particle into the [particleCount, particleCapacity) tail region.
#version 460

layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict buffer Particles {
    vec4 p[];
};

const int EMITTER_CAP = 8;

struct EmitterGpu {
    vec4 pos_radius;            // .xyz = pos; .w = radius
    vec4 vel_rate_age_pad;      // .xyz = velocity; .w = emission rate
};

layout(set=0, binding=1, std140) uniform U {
    uint        emitterCount;
    uint        particleCount;
    uint        presetParticleCount;
    uint        particleCapacity;
    float       particleRadius;
    float       dt;
    vec2        _pad;
    EmitterGpu  emitters[EMITTER_CAP];
};

uint pcg_hash(uint v) {
    uint state = v * 747796405u + 2891336453u;
    uint word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

vec3 unit_ball_sample(uint seed) {
    float u1 = float(pcg_hash(seed + 0u)) / 4294967295.0;
    float u2 = float(pcg_hash(seed + 1u)) / 4294967295.0;
    float u3 = float(pcg_hash(seed + 2u)) / 4294967295.0;
    vec3  v  = vec3(u1, u2, u3) * 2.0 - vec3(1.0);
    float len = length(v);
    return (len > 1.0) ? (v / len) : v;
}

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (emitterCount == 0u) return;

    uint emitter_idx = gid % emitterCount;
    EmitterGpu e     = emitters[emitter_idx];

    uint slot = particleCount + gid;
    if (slot >= particleCapacity) return;

    vec3 offset = unit_ball_sample(slot * 7919u + emitter_idx * 31u) * e.pos_radius.w;
    vec3 pos    = e.pos_radius.xyz + offset;
    vec3 vel    = e.vel_rate_age_pad.xyz;

    p[slot * 8u + 0u] = vec4(pos, particleRadius);
    p[slot * 8u + 1u] = vec4(vel, 0.0);
    p[slot * 8u + 2u] = vec4(0.0);
    p[slot * 8u + 3u] = vec4(0.0);
    p[slot * 8u + 5u] = uintBitsToFloat(uvec4(slot, 0u, 1u, 0u));
    p[slot * 8u + 6u] = vec4(0.0);
    p[slot * 8u + 7u] = vec4(0.0);
}
