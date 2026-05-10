// prefix_sum_addback.compute.wgsl — Spatial-hash counting-sort, pass C of 3 (scan phase).
//
// Block-sum add-back. Each thread adds block_sums[blockIdx] to its
// corresponding cell_counts[c] entry (which currently holds the LOCAL
// exclusive prefix from pass A) and writes the result to cell_starts[c].
//
// === Post-condition (architect-2 invariant) ===
// After this pass, for c in [0, cellCount):
//   cell_starts[c]  =  sum_{i in [0, c)} (original_cell_counts[i])
//
// The sentinel slot cell_starts[cellCount] = entityCount is written once per
// frame from main.ts via device.queue.writeBuffer (NOT computed in this
// shader — pass A has already overwritten cell_counts so the original
// per-slot counts are no longer available, and recovering the total here
// would require additional bookkeeping). The CPU-side sentinel write is a
// 4-byte queue.writeBuffer call per frame, negligible cost. This sentinel
// makes the cell-walk loops in flock_update / predator_update index-safe
// at the boundary without per-loop special-case fallbacks; matches GPU-sort
// convention (CUB / RocPRIM) of N+1 prefix slots with sentinel.
//
// Note: cell_counts is NOT preserved by this pass — we read its value but
// the buffer is not used again until next frame's clear+pass-1 chain, so the
// read-only-storage binding is fine.
//
// === Workgroup size ===
// 256. Dispatch grid: ceil(cellCount / 256) workgroups. At 512 cells, that's 2.

@group(0) @binding(0) var<storage, read>       cell_counts: array<u32>;
@group(0) @binding(1) var<storage, read>       block_sums:  array<u32>;
@group(0) @binding(2) var<storage, read_write> cell_starts: array<u32>;

@compute @workgroup_size(256, 1, 1)
fn cs_main(
    @builtin(global_invocation_id) gid:  vec3<u32>,
    @builtin(workgroup_id)         wgid: vec3<u32>,
) {
    let c = gid.x;
    let cell_count_total = arrayLength(&cell_counts);
    if (c >= cell_count_total) { return; }

    cell_starts[c] = cell_counts[c] + block_sums[wgid.x];
}
