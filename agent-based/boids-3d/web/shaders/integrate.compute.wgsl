// integrate.compute.wgsl — apply velocity to position, enforce box bounds.
//
// For each entity in [0, params.entityCount), advance position by velocity
// (step size 1.0 frame) and reflect at the box walls (bounce-back: position
// clamped, velocity component flipped on the impacted axis).
//
// === Single read_write binding — deliberate exception to read-old-write-new ===
// All other neighbor-reading passes (cell_count, scatter, flock_update,
// predator_update) read from "old" and write to "new" via distinct storage
// buffers. Integrate is the ONE pass that reads and writes the same buffer:
// each thread reads entity[i], modifies in place, writes entity[i] — no
// neighbor reads, no cross-thread aliasing. WebGPU validation requires the
// SAME binding for read+write of the same buffer (separate read-only-storage
// + storage bindings of the same buffer in one bind group is rejected).
//
// This pass runs AFTER flock_update + predator_update have written all
// new velocities. Reading entity[i] here gets the just-written velocity
// (memory ordering across compute pass boundaries is guaranteed by WebGPU
// dispatch sequencing).

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

@group(0) @binding(0) var<uniform>             params:   Params;
@group(0) @binding(1) var<storage, read_write> entities: array<Entity>;

@compute @workgroup_size(256, 1, 1)
fn cs_main(@builtin(global_invocation_id) gid: vec3<u32>) {
    let i = gid.x;
    if (i >= params.entityCount) { return; }

    var e = entities[i];
    e.pos = e.pos + e.velocity;

    let h = params.boxHalfExtent;
    if (e.pos.x >  h) { e.pos.x =  h; e.velocity.x = -abs(e.velocity.x); }
    if (e.pos.x < -h) { e.pos.x = -h; e.velocity.x =  abs(e.velocity.x); }
    if (e.pos.y >  h) { e.pos.y =  h; e.velocity.y = -abs(e.velocity.y); }
    if (e.pos.y < -h) { e.pos.y = -h; e.velocity.y =  abs(e.velocity.y); }
    if (e.pos.z >  h) { e.pos.z =  h; e.velocity.z = -abs(e.velocity.z); }
    if (e.pos.z < -h) { e.pos.z = -h; e.velocity.z =  abs(e.velocity.z); }

    entities[i] = e;
}
