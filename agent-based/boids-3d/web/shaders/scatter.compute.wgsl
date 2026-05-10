// scatter.compute.wgsl — Spatial-hash counting-sort, pass 3 of 3 (sort phase).
//
// For each entity in [0, params.entityCount), recompute its home cell index,
// claim a slot via atomicAdd on scratch_counter[cellIdx], and write the
// entity's index into sorted_entity_indices at offset (cell_starts[c] + slot).
//
// === Post-condition (architect-2 invariant) ===
// After this pass, sorted_entity_indices[k] for k in [cell_starts[c],
// cell_starts[c] + cell_starts_next - cell_starts[c]) holds the indices of
// entities whose home cell is c, in arbitrary intra-cell order. Equivalently:
//   for every entity i with home cell c(i), there exists exactly one slot
//   k in [cell_starts[c(i)], cell_starts[c(i) + 1]) such that
//   sorted_entity_indices[k] == i.
//
// === Pre-conditions ===
//   - cell_starts[c] holds the global exclusive prefix-sum of original cell
//     counts (pass C's output).
//   - scratch_counter is cleared to zero (encoder.clearBuffer in main.ts
//     before this dispatch). The kernel atomicAdds into it; without the
//     clear, slots collide.
//
// Architect-2 verification: scratch_counter and cell_counts are distinct
// buffers. cell_counts was overwritten in passes A + C with prefix-sum
// values; reusing it as the scatter scratch counter would corrupt the
// post-condition. Verify main.ts allocates them as separate buffers.

struct Params {
    boidCount: u32, predatorCount: u32, entityCount: u32, iteration: u32,
    cellCount: u32, gridDim: u32,
    cellSize: f32, boxHalfExtent: f32,
    separationRadius: f32, separationWeight: f32,
    alignmentRadius:  f32, alignmentWeight:  f32,
    cohesionRadius:   f32, cohesionWeight:   f32,
    boidMaxSpeed:     f32,
    leaderCount: u32, leaderInfluenceRadius: f32, leaderStrength: f32,
    predatorMode: u32, predatorFleeRadius: f32, predatorFleeStrength: f32,
    predatorDetectionRadius: f32, predatorRePickFrames: u32, predatorSpeedMul: f32,
    boidScale: f32,
    _pad0: f32, _pad1: f32, _pad2: f32,
    boidColor:     vec3<f32>,
    predatorColor: vec3<f32>,
    leaderColor:   vec3<f32>,
}

struct Entity {
    pos: vec3<f32>, species: u32,
    velocity: vec3<f32>, _pad: f32,
}

@group(0) @binding(0) var<uniform>             params:               Params;
@group(0) @binding(1) var<storage, read>       old_entities:         array<Entity>;
@group(0) @binding(2) var<storage, read>       cell_starts:          array<u32>;
@group(0) @binding(3) var<storage, read_write> scratch_counter:      array<atomic<u32>>;
@group(0) @binding(4) var<storage, read_write> sorted_entity_indices: array<u32>;

fn cell_index(pos: vec3<f32>) -> u32 {
    let lx = (pos.x + params.boxHalfExtent) / params.cellSize;
    let ly = (pos.y + params.boxHalfExtent) / params.cellSize;
    let lz = (pos.z + params.boxHalfExtent) / params.cellSize;
    let gd = i32(params.gridDim);
    let cx = clamp(i32(floor(lx)), 0, gd - 1);
    let cy = clamp(i32(floor(ly)), 0, gd - 1);
    let cz = clamp(i32(floor(lz)), 0, gd - 1);
    return u32(cz) * params.gridDim * params.gridDim
         + u32(cy) * params.gridDim
         + u32(cx);
}

@compute @workgroup_size(256, 1, 1)
fn cs_main(@builtin(global_invocation_id) gid: vec3<u32>) {
    let i = gid.x;
    if (i >= params.entityCount) { return; }

    let e = old_entities[i];
    let c = cell_index(e.pos);
    let slot = atomicAdd(&scratch_counter[c], 1u);
    sorted_entity_indices[cell_starts[c] + slot] = i;
}
