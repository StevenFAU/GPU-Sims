// prefix_sum_block_l2.comp.glsl — Second-level recursive scan over per-chunk
// totals produced by prefix_sum_block's SCAN_ONLY mode. Dispatched only when
// num_blocks > WG_SIZE; covers up to WG_SIZE^3 = 16M cells (~254 per axis).
#version 460

#define WG_SIZE 256

layout(local_size_x = WG_SIZE) in;

layout(set=0, binding=0, std430) restrict readonly buffer InL2Sums {
    uint l2_sums[];
};
layout(set=0, binding=1, std430) restrict writeonly buffer OutL2Prefixes {
    uint l2_prefixes[];
};
layout(set=0, binding=2, std140) uniform U {
    uint particleCount;
    uint _pad0;
    uint totalCells;
    uint _pad1;
    vec4 _pad2; vec4 _pad3; vec4 _pad4;
};

shared uint sdata[WG_SIZE];

void main() {
    uint tid = gl_LocalInvocationID.x;
    uint num_blocks    = (totalCells + WG_SIZE - 1u) / WG_SIZE;
    uint num_l2_blocks = (num_blocks + WG_SIZE - 1u) / WG_SIZE;

    sdata[tid] = (tid < num_l2_blocks) ? l2_sums[tid] : 0u;
    barrier();

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
    if (tid == 0u) sdata[WG_SIZE - 1u] = 0u;
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
    if (tid < num_l2_blocks) {
        l2_prefixes[tid] = sdata[tid];
    }
}
