// prefix_sum_block.compute.wgsl — Spatial-hash counting-sort, pass B of 3 (scan phase).
//
// Block-sums scan. Single workgroup performs an exclusive Blelloch scan on
// the block_sums[] buffer (2 elements at boids' 512-cell scale; kernel sized
// for up to 256 elements to support future sims with more cells).
//
// === Post-condition (architect-2 invariant) ===
// After this pass:
//   block_sums[b]  =  sum_{i in [0, b)} (original_block_sums[i])
//                     (exclusive prefix-sum across blocks)
//
// At boids' scale this is trivially small (2 elements), but the kernel uses
// the full Blelloch pattern so the same shader works at larger cell counts.
//
// === Workgroup size ===
// 256 threads; only the first N (= block count) participate. At 512 cells with
// 256-element blocks, N = 2. Dispatch grid: 1 workgroup of 256 threads.

@group(0) @binding(0) var<storage, read_write> block_sums: array<u32>;

const N: u32 = 256u;   // max block count this kernel supports

var<workgroup> shared_data: array<u32, 256>;

@compute @workgroup_size(256, 1, 1)
fn cs_main(@builtin(local_invocation_id) lid: vec3<u32>) {
    let tid = lid.x;
    let block_count = arrayLength(&block_sums);

    // Load — out-of-range slots load zero.
    if (tid < block_count) {
        shared_data[tid] = block_sums[tid];
    } else {
        shared_data[tid] = 0u;
    }
    workgroupBarrier();

    // Up-sweep.
    var stride: u32 = 1u;
    while (stride < N) {
        let idx = (tid + 1u) * stride * 2u - 1u;
        if (idx < N) {
            shared_data[idx] = shared_data[idx] + shared_data[idx - stride];
        }
        stride = stride * 2u;
        workgroupBarrier();
    }

    // Seed down-sweep with zero at the rightmost slot.
    if (tid == 0u) {
        shared_data[N - 1u] = 0u;
    }
    workgroupBarrier();

    // Down-sweep.
    stride = N / 2u;
    while (stride >= 1u) {
        let idx = (tid + 1u) * stride * 2u - 1u;
        if (idx < N) {
            let t = shared_data[idx - stride];
            shared_data[idx - stride] = shared_data[idx];
            shared_data[idx]          = shared_data[idx] + t;
        }
        stride = stride / 2u;
        workgroupBarrier();
    }

    // Write back.
    if (tid < block_count) {
        block_sums[tid] = shared_data[tid];
    }
}
