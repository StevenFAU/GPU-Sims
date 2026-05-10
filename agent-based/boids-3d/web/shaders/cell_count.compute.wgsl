// cell_count.compute.wgsl — Spatial-hash counting-sort, pass 1 of 3 (sort phase).
//
// For each entity in [0, params.entityCount), compute the home cell index from
// the entity's world-space position and atomically increment cell_counts[cellIdx].
// After this kernel, cell_counts[c] holds the number of entities whose home cell
// is c, for every c in [0, params.cellCount).
//
// Cell index encoding (same as main.ts, scatter, flock_update, predator_update):
//   cellX = clamp(floor((pos.x + boxHalfExtent) / cellSize), 0, gridDim - 1)
//   cellY, cellZ analogous
//   cellIdx = cellZ * gridDim^2 + cellY * gridDim + cellX
//
// Architect-2 invariant: cell_counts is cleared to zero by encoder.clearBuffer()
// in main.ts BEFORE this pass dispatches. This kernel does NOT clear; it only
// increments. Verify the clear is present and ordered before this dispatch.

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
    pos: vec3<f32>,
    species: u32,
    velocity: vec3<f32>,
    _pad: f32,
}

@group(0) @binding(0) var<uniform>            params:       Params;
@group(0) @binding(1) var<storage, read>      old_entities: array<Entity>;
@group(0) @binding(2) var<storage, read_write> cell_counts: array<atomic<u32>>;

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
    atomicAdd(&cell_counts[c], 1u);
}
