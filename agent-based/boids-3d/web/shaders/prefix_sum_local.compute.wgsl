// prefix_sum_local.compute.wgsl — Spatial-hash counting-sort, pass A of 3 (scan phase).
//
// Multi-block exclusive prefix scan, local pass. Each workgroup of 256 threads
// performs a Blelloch up-sweep + down-sweep on its 256-element chunk of
// cell_counts in shared memory, writing the local exclusive prefix-sum back
// into cell_counts[c] (in-place) and the block's total to block_sums[blockIdx].
//
// === Post-condition (architect-2 invariant) ===
// After this pass:
//   cell_counts[c]  =  sum_{i in [block * BLOCK, block * BLOCK + tid)} (
//                          original_cell_counts[i]
//                      )
//                      where block = c / BLOCK, tid = c % BLOCK
//                      (i.e. exclusive prefix-sum within the block)
//   block_sums[block] = sum_{i in [block * BLOCK, (block + 1) * BLOCK)} (
//                          original_cell_counts[i]
//                      )
//                      (the block's total)
//
// This is the standard Blelloch scan formulation. Up-sweep reduces in pairs
// at increasing strides; down-sweep distributes prefix sums back down.
//
// === Workgroup size and block count ===
//
// Workgroup size = 256 = CELL_PREFIX_BLOCK_SIZE. Cell count = 512. Block count
// = 2. Dispatch grid: 2 workgroups, each handling 256 cells. The kernel reads
// the entity-count-derived cell_counts buffer atomicLoad-equivalent values
// (after pass cell_count's atomic writes have completed; the prior dispatch
// boundary acts as a barrier).

@group(0) @binding(0) var<storage, read_write> cell_counts: array<u32>;
@group(0) @binding(1) var<storage, read_write> block_sums:  array<u32>;

const BLOCK: u32 = 256u;

var<workgroup> shared_data: array<u32, 256>;

@compute @workgroup_size(256, 1, 1)
fn cs_main(
    @builtin(global_invocation_id) gid:  vec3<u32>,
    @builtin(local_invocation_id)  lid:  vec3<u32>,
    @builtin(workgroup_id)         wgid: vec3<u32>,
) {
    let tid    = lid.x;
    let global = gid.x;

    // Load element into shared memory. Out-of-range slots load zero so the
    // scan's prefix is still correct (zeros don't contribute).
    shared_data[tid] = cell_counts[global];
    workgroupBarrier();

    // Up-sweep (reduce). Pairs sum into the right index of each pair, doubling
    // stride each step. After log2(BLOCK) steps, shared_data[BLOCK-1] holds
    // the sum of the entire block.
    var stride: u32 = 1u;
    while (stride < BLOCK) {
        let idx = (tid + 1u) * stride * 2u - 1u;
        if (idx < BLOCK) {
            shared_data[idx] = shared_data[idx] + shared_data[idx - stride];
        }
        stride = stride * 2u;
        workgroupBarrier();
    }

    // Save the block total to block_sums and seed the down-sweep with zero
    // at the rightmost slot (this is what makes the result EXCLUSIVE prefix).
    if (tid == 0u) {
        block_sums[wgid.x] = shared_data[BLOCK - 1u];
        shared_data[BLOCK - 1u] = 0u;
    }
    workgroupBarrier();

    // Down-sweep. Reverse direction; each step halves the stride. At each
    // pair, the right element receives left+right and the left element
    // takes the right's old value. After log2(BLOCK) steps, shared_data
    // holds the exclusive prefix-sum.
    stride = BLOCK / 2u;
    while (stride >= 1u) {
        let idx = (tid + 1u) * stride * 2u - 1u;
        if (idx < BLOCK) {
            let t = shared_data[idx - stride];
            shared_data[idx - stride] = shared_data[idx];
            shared_data[idx]          = shared_data[idx] + t;
        }
        stride = stride / 2u;
        workgroupBarrier();
    }

    // Write back the local exclusive prefix-sum.
    cell_counts[global] = shared_data[tid];
}
