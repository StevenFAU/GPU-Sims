// flock_update.compute.wgsl — boid kernel, applies Reynolds + leader + predator-flee forces.
//
// For each boid in [0, params.boidCount), accumulate:
//   1. Reynolds separation, alignment, cohesion over boid neighbors in the
//      27-cell walk (filtered by species == 0u).
//   2. Leader attraction: walk the leaders array directly (not via spatial
//      hash; cap 32 leaders × 100k boids = 3.2M ops/frame, negligible).
//      Cosine-envelope falloff over leaderInfluenceRadius.
//   3. Predator flee: same 27-cell walk as Reynolds, filtered by species == 1u.
//      Linear falloff over predatorFleeRadius (sharper edge than cosine).
//
// Final velocity = old_velocity + (separation + alignment + cohesion + leader
//                                   + predator_flee), clamped to boidMaxSpeed.
// Position is NOT advanced here; the integrate pass owns position updates.
// flock_update writes: new_entities[i].velocity (for i < boidCount)
//                      new_entities[i].pos      (copied unchanged from old; integrate moves it)
//                      new_entities[i].species  (copied unchanged)
//                      new_entities[i]._pad     (zeroed)
//
// === Read-old-write-new invariant (architect-2 callout #6) ===
//   - old_entities is bound as <storage, read>: cannot accidentally write to it.
//   - new_entities is bound as <storage, read_write>: writes only to slot i,
//     never to neighbor slots.
//   - sorted_entity_indices and cell_starts are bound as <storage, read>:
//     consumed but not modified.
// Verify: no expression of the form `new_entities[neighbor_idx] = ...`
// appears anywhere in this kernel — only `new_entities[i] = ...`.
//
// === 27-cell walk invariant (architect-2 callout #2) ===
// The kernel walks the 27 cells in the 3×3×3 region around the boid's home
// cell. cellSize is locked at 4.0u, and every neighborhood-radius slider is
// clamped to ≤ cellSize at the panel layer (main.ts), so the 27-cell walk
// finds every neighbor at any used radius. If any radius slider's max ever
// exceeds cellSize, this kernel silently misses neighbors at the upper range.

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

struct Leader {
    position: vec3<f32>,
    strength: f32,
}

@group(0) @binding(0) var<uniform>             params:                Params;
@group(0) @binding(1) var<storage, read>       old_entities:          array<Entity>;
@group(0) @binding(2) var<storage, read_write> new_entities:          array<Entity>;
@group(0) @binding(3) var<storage, read>       sorted_entity_indices: array<u32>;
@group(0) @binding(4) var<storage, read>       cell_starts:           array<u32>;
@group(0) @binding(5) var<storage, read>       leaders:               array<Leader>;

const PI: f32 = 3.14159265359;

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

@compute @workgroup_size(256, 1, 1)
fn cs_main(@builtin(global_invocation_id) gid: vec3<u32>) {
    let i = gid.x;
    if (i >= params.boidCount) { return; }

    let me = old_entities[i];

    // Reynolds-rule accumulators.
    var sep_force = vec3<f32>(0.0);   var sep_count: u32 = 0u;
    var align_v   = vec3<f32>(0.0);   var align_count: u32 = 0u;
    var cohesion_pos = vec3<f32>(0.0); var cohesion_count: u32 = 0u;
    // Predator-flee accumulator.
    var flee_force = vec3<f32>(0.0);

    let sep_r2     = params.separationRadius * params.separationRadius;
    let align_r2   = params.alignmentRadius  * params.alignmentRadius;
    let cohesion_r2 = params.cohesionRadius  * params.cohesionRadius;
    let flee_r     = params.predatorFleeRadius;
    let flee_r2    = flee_r * flee_r;

    let home = cell_index_of(me.pos);
    let gd_i = i32(params.gridDim);

    // 27-cell walk (3×3×3 neighborhood).
    for (var dz: i32 = -1; dz <= 1; dz = dz + 1) {
        for (var dy: i32 = -1; dy <= 1; dy = dy + 1) {
            for (var dx: i32 = -1; dx <= 1; dx = dx + 1) {
                let cx = home.x + dx;
                let cy = home.y + dy;
                let cz = home.z + dz;
                if (cx < 0 || cx >= gd_i ||
                    cy < 0 || cy >= gd_i ||
                    cz < 0 || cz >= gd_i) { continue; }
                let cell = cell_linear(vec3<i32>(cx, cy, cz));
                let begin = cell_starts[cell];
                let end_idx = cell_starts[cell + 1u];

                for (var k: u32 = begin; k < end_idx; k = k + 1u) {
                    let j = sorted_entity_indices[k];
                    if (j == i) { continue; }   // skip self
                    let other = old_entities[j];
                    let diff = me.pos - other.pos;
                    let d2 = dot(diff, diff);

                    if (other.species == 0u) {
                        // Reynolds rules with a fellow boid.
                        if (d2 > 0.0 && d2 < sep_r2) {
                            // Separation force, canonical Reynolds 1987 formulation:
                            //   force = (me - other) / d^2
                            // The 1/d^2 falloff (closer = stronger) produces the
                            // recognizable near-miss recovery dynamic. Epsilon
                            // (0.01u^2) prevents the d -> 0 singularity without
                            // affecting behavior at meaningful distances.
                            sep_force = sep_force + diff / max(d2, 0.01);
                            sep_count = sep_count + 1u;
                        }
                        if (d2 < align_r2) {
                            align_v = align_v + other.velocity;
                            align_count = align_count + 1u;
                        }
                        if (d2 < cohesion_r2) {
                            cohesion_pos = cohesion_pos + other.pos;
                            cohesion_count = cohesion_count + 1u;
                        }
                    } else {
                        // Predator: linear-falloff flee force (sharper edge than cosine).
                        if (d2 < flee_r2 && d2 > 0.0) {
                            let r = sqrt(d2);
                            let mag = params.predatorFleeStrength * (1.0 - r / flee_r);
                            flee_force = flee_force + (diff / r) * mag;
                        }
                    }
                }
            }
        }
    }

    // Reynolds force computation.
    var sep_v = vec3<f32>(0.0);
    if (sep_count > 0u) {
        sep_v = sep_force * (params.separationWeight / f32(sep_count));
    }
    var align_steer = vec3<f32>(0.0);
    if (align_count > 0u) {
        let avg = align_v / f32(align_count);
        align_steer = (avg - me.velocity) * params.alignmentWeight;
    }
    var cohesion_steer = vec3<f32>(0.0);
    if (cohesion_count > 0u) {
        let center = cohesion_pos / f32(cohesion_count);
        cohesion_steer = (center - me.pos) * params.cohesionWeight;
    }

    // Leader attraction: walk the leaders array directly (cap 32; cheaper than spatial hash).
    // Cosine-envelope falloff: force_mag = strength * cos((π/2) * r / R) for r < R.
    var leader_force = vec3<f32>(0.0);
    let lr = params.leaderInfluenceRadius;
    if (lr > 0.0) {
        for (var li: u32 = 0u; li < params.leaderCount; li = li + 1u) {
            let lead = leaders[li];
            let to_leader = lead.position - me.pos;
            let dl = length(to_leader);
            if (dl > 0.0 && dl < lr) {
                let env = cos((PI * 0.5) * (dl / lr));    // cosine envelope, peaks at r=0
                let mag = lead.strength * params.leaderStrength * env;
                leader_force = leader_force + (to_leader / dl) * mag;
            }
        }
    }

    // Combine all forces, integrate velocity, clamp speed.
    var new_v = me.velocity + sep_v + align_steer + cohesion_steer + leader_force + flee_force;
    let speed = length(new_v);
    if (speed > params.boidMaxSpeed) {
        new_v = new_v * (params.boidMaxSpeed / max(speed, 1e-6));
    }

    // Write new_entities[i] — pos copied unchanged (integrate advances it),
    // velocity is the new clamped value.
    var out: Entity;
    out.pos = me.pos;
    out.species = me.species;
    out.velocity = new_v;
    out._pad = 0.0;
    new_entities[i] = out;
}
