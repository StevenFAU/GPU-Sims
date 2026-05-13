// prefix_sum_local.comp.glsl — Per-block Blelloch exclusive scan over cell_counts.
// Translated from agent-based/boids-3d/web/shaders/prefix_sum_local.compute.wgsl.
#version 460

#define WG_SIZE 256

layout(local_size_x = WG_SIZE) in;

layout(set=0, binding=0, std430) restrict readonly buffer InCounts {
    uint counts[];
};
layout(set=0, binding=1, std430) restrict writeonly buffer OutPerBlockPrefixes {
    uint per_block[];
};
layout(set=0, binding=2, std430) restrict writeonly buffer OutBlockSums {
    uint block_sums[];
};
layout(set=0, binding=3, std140) uniform U {
    uint particleCount;
    uint _pad0; uint totalCells; uint _pad1;
    vec4 _pad2; vec4 _pad3; vec4 _pad4;
};

shared uint sdata[WG_SIZE];

void main() {
    uint tid   = gl_LocalInvocationID.x;
    uint gid   = gl_GlobalInvocationID.x;
    uint block = gl_WorkGroupID.x;

    sdata[tid] = (gid < totalCells) ? counts[gid] : 0u;
    barrier();

    // Blelloch up-sweep (reduce phase): build a sum tree.
    uint offset = 1u;
    for (uint d = WG_SIZE >> 1; d > 0u; d >>= 1u) {
        barrier();
        if (tid < d) {
            uint ai = offset * (2u * tid + 1u) - 1u;
            uint bi = offset * (2u * tid + 2u) - 1u;
            sdata[bi] += sdata[ai];
        }
        offset *= 2u;
    }

    // Save block total + clear last element for exclusive-scan semantics.
    if (tid == 0u) {
        block_sums[block]   = sdata[WG_SIZE - 1u];
        sdata[WG_SIZE - 1u] = 0u;
    }

    // Blelloch down-sweep (distribute phase).
    for (uint d = 1u; d < WG_SIZE; d *= 2u) {
        offset >>= 1u;
        barrier();
        if (tid < d) {
            uint ai = offset * (2u * tid + 1u) - 1u;
            uint bi = offset * (2u * tid + 2u) - 1u;
            uint t  = sdata[ai];
            sdata[ai] = sdata[bi];
            sdata[bi] += t;
        }
    }

    barrier();
    if (gid < totalCells) {
        per_block[gid] = sdata[tid];
    }
}
