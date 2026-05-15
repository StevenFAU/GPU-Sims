#version 460
#extension GL_GOOGLE_include_directive : require

// Streamline advection — one thread per streamline. RK2 step using trilinearly-
// sampled velocity from the moment buffer. Each streamline owns a ring buffer
// of (xyz, age) positions; this kernel writes the new position to the ring head
// each render frame. Aged-out or out-of-domain streamlines are reseeded to a
// random position in an inlet-aligned slab.
//
// The velocity_field sampler must be created with linear (trilinear) filter +
// clamp-to-edge address mode.

layout(local_size_x = 64) in;

layout(set = 0, binding = 0)         uniform sampler3D velocity_field;
layout(set = 0, binding = 1, std430) buffer  PositionHistory {
    vec4 positions[];   // length = streamline_count * history; (xyz, age)
};

layout(set = 0, binding = 2) uniform StreamlineAdvectUniforms {
    ivec4    dims;
    vec4     domain_min;
    vec4     domain_max;
    uint     streamline_count;
    uint     history;
    uint     head_index;
    uint     frame_count;
    float    dt_render;
    uint     reseed_age_threshold;
    uint     _pad0;
    uint     _pad1;
} U;

uint xorshift(inout uint state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}
float rng01(inout uint state) {
    return float(xorshift(state) & 0x00FFFFFFu) / float(0x01000000u);
}

void main() {
    uint sid = gl_GlobalInvocationID.x;
    if (sid >= U.streamline_count) return;

    uint prev_head = (U.head_index + U.history - 1u) % U.history;
    vec4 prev = positions[sid * U.history + prev_head];
    vec3 pos  = prev.xyz;
    float age = prev.w;

    bool out_of_bounds = any(lessThan(pos, U.domain_min.xyz))
                      || any(greaterThan(pos, U.domain_max.xyz));

    bool reseeding_this_frame = false;
    if (age >= float(U.reseed_age_threshold) || out_of_bounds) {
        // Seed ensures distinct streams per (sid, frame_count) pair.
        uint state = sid * 2654435761u ^ U.frame_count * 1597334677u;
        // Inlet-aligned slab: x in [Nx/16, Nx/8], y/z uniform across domain.
        float xs = U.domain_min.x + (U.domain_max.x - U.domain_min.x) * 0.0625
                 * (1.0 + rng01(state));
        float ys = U.domain_min.y + (U.domain_max.y - U.domain_min.y) * rng01(state);
        float zs = U.domain_min.z + (U.domain_max.z - U.domain_min.z) * rng01(state);
        pos = vec3(xs, ys, zs);
        age = 0.0;
        reseeding_this_frame = true;
    } else {
        // RK2 step in lattice units. dt_render is the per-frame time step
        // (clamped at the host).
        vec3 ctr1 = pos / vec3(U.dims.xyz);
        vec3 u1   = texture(velocity_field, ctr1).xyz;
        vec3 mid  = pos + 0.5 * U.dt_render * u1;
        vec3 ctr2 = mid / vec3(U.dims.xyz);
        vec3 u2   = texture(velocity_field, ctr2).xyz;
        pos += U.dt_render * u2;
        age += 1.0;
    }

    if (reseeding_this_frame) {
        // Fill ALL history slots with the new seed position so the line-strip
        // vertex shader doesn't draw a teleport-jump from the new position to
        // the 63 stale positions left over from the previous incarnation
        // (visible as bright "white flash" sheets).
        vec4 v = vec4(pos, age);
        for (uint h = 0u; h < U.history; ++h) {
            positions[sid * U.history + h] = v;
        }
    } else {
        positions[sid * U.history + U.head_index] = vec4(pos, age);
    }
}
