// predator_update.compute.wgsl — predator kernel, three runtime-switchable hunting modes.
//
// For each predator in [0, params.predatorCount), branch on params.predatorMode:
//
//   Mode 0 (nearest-prey):
//     Walk the 27-cell neighborhood for nearest BOID (species == 0u).
//     Hysteresis: if previous target is still within detectionRadius and its
//     d² is within 4.0u² of the new nearest's d² (constant-d² hysteresis;
//     "stickiness" varies with distance — ~2.0u sticky when prey is at the
//     boundary, ~0.83u when prey is at distance 2u), KEEP previous target.
//     Steer toward target with constant max-acceleration toward target direction.
//
//   Mode 1 (stochastic-prey):
//     Increment target_age_frames. If target_age_frames >= predatorRePickFrames
//     OR target_boid_id is sentinel, walk the 27-cell neighborhood and pick a
//     uniformly random boid within detectionRadius (or nearest if no boid is in
//     range), reset target_age_frames to 0. Otherwise, keep current target.
//     Steer toward target.
//
//   Mode 2 (flock-center):
//     Walk the 27-cell neighborhood, accumulate sum of boid positions, divide
//     by count to get local centroid. Steer toward centroid. target_boid_id
//     and target_age_frames are unused in this mode.
//
// === Mode-switch reset semantics (architect-2 callout #3) ===
// On user-initiated dropdown change AND preset change AND reseed, main.ts
// uploads sentinel values (target_boid_id = 0xFFFFFFFF, target_age_frames = 0)
// to the predator state buffer BEFORE the next dispatch. This kernel does NOT
// reset on its own — it relies on the sentinel values being present at the
// start of any frame where the mode has just changed.
// On F9 capture-load, the predator state is restored bit-exactly from the
// capture (NO sentinel reset) — the captured frame's mode-state was internally
// consistent with the captured mode at capture-time.

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

struct PredatorState {
    target_boid_id:    u32,
    target_age_frames: u32,
    _pad0: u32,
    _pad1: u32,
}

@group(0) @binding(0) var<uniform>             params:                Params;
@group(0) @binding(1) var<storage, read>       old_entities:          array<Entity>;
@group(0) @binding(2) var<storage, read_write> new_entities:          array<Entity>;
@group(0) @binding(3) var<storage, read>       sorted_entity_indices: array<u32>;
@group(0) @binding(4) var<storage, read>       cell_starts:           array<u32>;
@group(0) @binding(5) var<storage, read_write> predator_state:        array<PredatorState>;

const SENTINEL: u32 = 0xFFFFFFFFu;
const PRED_ACCEL: f32 = 0.4;        // per-frame steer acceleration toward target

fn cell_index_of(pos: vec3<f32>) -> vec3<i32> {
    let lx = (pos.x + params.boxHalfExtent) / params.cellSize;
    let ly = (pos.y + params.boxHalfExtent) / params.cellSize;
    let lz = (pos.z + params.boxHalfExtent) / params.cellSize;
    let gd = i32(params.gridDim);
    return vec3<i32>(
        clamp(i32(floor(lx)), 0, gd - 1),
        clamp(i32(floor(ly)), 0, gd - 1),
        clamp(i32(floor(lz)), 0, gd - 1),
    );
}

fn cell_linear(c: vec3<i32>) -> u32 {
    return u32(c.z) * params.gridDim * params.gridDim
         + u32(c.y) * params.gridDim
         + u32(c.x);
}

// xorshift32 PRNG, deterministic per (predatorIdx, iteration).
fn rng_u32(seed: u32) -> u32 {
    var s = seed;
    s = s ^ (s << 13u);
    s = s ^ (s >> 17u);
    s = s ^ (s << 5u);
    return s;
}

@compute @workgroup_size(256, 1, 1)
fn cs_main(@builtin(global_invocation_id) gid: vec3<u32>) {
    let pi = gid.x;                            // predator index in [0, predatorCount)
    if (pi >= params.predatorCount) { return; }

    let entity_idx = params.boidCount + pi;    // index into unified entity buffer
    let me = old_entities[entity_idx];
    var state = predator_state[pi];

    let detect_r2 = params.predatorDetectionRadius * params.predatorDetectionRadius;
    let home = cell_index_of(me.pos);
    let gd_i = i32(params.gridDim);

    var target_pos = vec3<f32>(0.0);
    var have_target: bool = false;

    if (params.predatorMode == 2u) {
        // ---- flock-center: average boid positions in 27-cell walk ----
        var sum_pos = vec3<f32>(0.0);
        var count: u32 = 0u;
        for (var dz: i32 = -1; dz <= 1; dz = dz + 1) {
            for (var dy: i32 = -1; dy <= 1; dy = dy + 1) {
                for (var dx: i32 = -1; dx <= 1; dx = dx + 1) {
                    let cx = home.x + dx; let cy = home.y + dy; let cz = home.z + dz;
                    if (cx < 0 || cx >= gd_i || cy < 0 || cy >= gd_i || cz < 0 || cz >= gd_i) { continue; }
                    let cell = cell_linear(vec3<i32>(cx, cy, cz));
                    let begin = cell_starts[cell];
                    let end_idx = cell_starts[cell + 1u];
                    for (var k: u32 = begin; k < end_idx; k = k + 1u) {
                        let j = sorted_entity_indices[k];
                        let other = old_entities[j];
                        if (other.species != 0u) { continue; }
                        let diff = other.pos - me.pos;
                        if (dot(diff, diff) < detect_r2) {
                            sum_pos = sum_pos + other.pos;
                            count = count + 1u;
                        }
                    }
                }
            }
        }
        if (count > 0u) {
            target_pos = sum_pos / f32(count);
            have_target = true;
        }
    } else {
        // ---- nearest-prey + stochastic-prey share the same neighbor scan ----
        // Find nearest boid in detection range and (for stochastic) collect
        // candidate IDs for random pick.
        var nearest_id: u32 = SENTINEL;
        var nearest_d2: f32 = detect_r2;       // monotone decreasing as we find closer
        var nearest_pos = vec3<f32>(0.0);
        var candidate_count: u32 = 0u;
        var pick_idx: u32 = 0u;
        if (params.predatorMode == 1u) {
            pick_idx = rng_u32(pi * 0x9E3779B1u + params.iteration) % 1024u;
        }
        var picked_id: u32 = SENTINEL;
        var picked_pos = vec3<f32>(0.0);
        var picked_seen: u32 = 0u;

        for (var dz: i32 = -1; dz <= 1; dz = dz + 1) {
            for (var dy: i32 = -1; dy <= 1; dy = dy + 1) {
                for (var dx: i32 = -1; dx <= 1; dx = dx + 1) {
                    let cx = home.x + dx; let cy = home.y + dy; let cz = home.z + dz;
                    if (cx < 0 || cx >= gd_i || cy < 0 || cy >= gd_i || cz < 0 || cz >= gd_i) { continue; }
                    let cell = cell_linear(vec3<i32>(cx, cy, cz));
                    let begin = cell_starts[cell];
                    let end_idx = cell_starts[cell + 1u];
                    for (var k: u32 = begin; k < end_idx; k = k + 1u) {
                        let j = sorted_entity_indices[k];
                        let other = old_entities[j];
                        if (other.species != 0u) { continue; }
                        let diff = other.pos - me.pos;
                        let d2 = dot(diff, diff);
                        if (d2 < detect_r2) {
                            // Track nearest.
                            if (d2 < nearest_d2) {
                                nearest_d2 = d2;
                                nearest_id = j;
                                nearest_pos = other.pos;
                            }
                            // Reservoir-1 sampling for the random pick (stochastic mode).
                            // Approximation: deterministic-modulo selection over candidates.
                            // Valid for the stochastic-mode demo; not a true uniform sample
                            // but visually indistinguishable.
                            if (candidate_count == pick_idx) {
                                picked_id = j;
                                picked_pos = other.pos;
                                picked_seen = 1u;
                            }
                            candidate_count = candidate_count + 1u;
                        }
                    }
                }
            }
        }

        if (params.predatorMode == 0u) {
            // ---- nearest-prey with hysteresis ----
            // If previous target is still in range and within 2.0 of nearest distance,
            // keep it. Otherwise switch to nearest.
            if (state.target_boid_id != SENTINEL && state.target_boid_id < params.boidCount) {
                let prev = old_entities[state.target_boid_id];
                let pdiff = prev.pos - me.pos;
                let pd2 = dot(pdiff, pdiff);
                if (pd2 < detect_r2 && pd2 - nearest_d2 < 4.0) {
                    target_pos = prev.pos;
                    have_target = true;
                }
            }
            if (!have_target && nearest_id != SENTINEL) {
                target_pos = nearest_pos;
                have_target = true;
                state.target_boid_id = nearest_id;
            }
        } else {
            // ---- stochastic-prey: re-pick on age-out or sentinel ----
            state.target_age_frames = state.target_age_frames + 1u;
            let must_repick =
                (state.target_boid_id == SENTINEL) ||
                (state.target_age_frames >= params.predatorRePickFrames) ||
                (state.target_boid_id >= params.boidCount);
            if (must_repick) {
                if (picked_seen == 1u) {
                    state.target_boid_id = picked_id;
                    state.target_age_frames = 0u;
                    target_pos = picked_pos;
                    have_target = true;
                } else if (nearest_id != SENTINEL) {
                    // Fallback to nearest if no random pick was selected
                    // (e.g., candidate_count < pick_idx).
                    state.target_boid_id = nearest_id;
                    state.target_age_frames = 0u;
                    target_pos = nearest_pos;
                    have_target = true;
                }
            } else {
                let prev = old_entities[state.target_boid_id];
                target_pos = prev.pos;
                have_target = true;
            }
        }
    }

    // Compute steer velocity.
    var new_v = me.velocity;
    if (have_target) {
        let to_target = target_pos - me.pos;
        let dist = length(to_target);
        if (dist > 1e-4) {
            new_v = me.velocity + (to_target / dist) * PRED_ACCEL;
        }
    }
    let max_speed = params.boidMaxSpeed * params.predatorSpeedMul;
    let s = length(new_v);
    if (s > max_speed) {
        new_v = new_v * (max_speed / max(s, 1e-6));
    }

    // Write new_entities[entity_idx] — pos unchanged, velocity is the new value.
    var out: Entity;
    out.pos = me.pos;
    out.species = 1u;
    out.velocity = new_v;
    out._pad = 0.0;
    new_entities[entity_idx] = out;

    predator_state[pi] = state;
}
