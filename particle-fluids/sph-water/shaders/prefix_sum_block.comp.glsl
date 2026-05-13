// prefix_sum_block.comp.glsl — Two-mode kernel via push-constant.
//
// SCAN_ONLY: Blelloch exclusive scan over a chunk of block_sums. When the
//   dispatch is multi-workgroup, also writes per-chunk total to l2_sums.
// ADDBACK_L2: adds the L2 prefix back into each element of this chunk's
//   block_prefixes, completing the two-level scan.
//
// When num_blocks <= WG_SIZE, a single SCAN_ONLY workgroup is dispatched and
// the L2 chain is skipped entirely (l2_sums[0] is dead data, harmless).
#version 460

#define WG_SIZE     256
#define SCAN_ONLY   0u
#define ADDBACK_L2  1u

layout(local_size_x = WG_SIZE) in;

layout(set=0, binding=0, std430) restrict buffer InOutSums {
    uint block_sums[];          // read in SCAN_ONLY; unused in ADDBACK_L2
};
layout(set=0, binding=1, std430) restrict buffer OutPrefixes {
    uint block_prefixes[];      // written in SCAN_ONLY; modified in-place in ADDBACK_L2
};
layout(set=0, binding=2, std430) restrict buffer OutL2Sums {
    uint l2_sums[];             // written in SCAN_ONLY (multi-WG); unused otherwise
};
layout(set=0, binding=3, std430) restrict readonly buffer InL2Prefixes {
    uint l2_prefixes[];         // read in ADDBACK_L2
};
layout(set=0, binding=4, std140) uniform U {
    uint particleCount;
    uint _pad0;
    uint totalCells;
    uint _pad1;
    vec4 _pad2; vec4 _pad3; vec4 _pad4;
};

layout(push_constant) uniform PC {
    uint mode;
} pc;

shared uint sdata[WG_SIZE];

void main() {
    uint tid        = gl_LocalInvocationID.x;
    uint wg         = gl_WorkGroupID.x;
    uint num_blocks = (totalCells + WG_SIZE - 1u) / WG_SIZE;
    uint chunk_base = wg * WG_SIZE;
    uint chunk_size = (chunk_base < num_blocks)
                        ? min(uint(WG_SIZE), num_blocks - chunk_base)
                        : 0u;

    if (pc.mode == ADDBACK_L2) {
        uint l2_offset = (wg > 0u) ? l2_prefixes[wg] : 0u;
        if (tid < chunk_size) {
            block_prefixes[chunk_base + tid] += l2_offset;
        }
        return;
    }

    // SCAN_ONLY: Blelloch over this chunk's block_sums.
    sdata[tid] = (tid < chunk_size) ? block_sums[chunk_base + tid] : 0u;
    barrier();

    // Up-sweep (reduce).
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

    // Capture chunk-total at the top of the up-sweep tree (sdata[WG_SIZE-1])
    // BEFORE down-sweep clears it.
    uint chunk_total = 0u;
    if (tid == 0u) {
        chunk_total          = sdata[WG_SIZE - 1u];
        sdata[WG_SIZE - 1u]  = 0u;
    }

    // Down-sweep (distribute).
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

    if (tid < chunk_size) {
        block_prefixes[chunk_base + tid] = sdata[tid];
    }
    if (tid == 0u) {
        l2_sums[wg] = chunk_total;
    }
}
