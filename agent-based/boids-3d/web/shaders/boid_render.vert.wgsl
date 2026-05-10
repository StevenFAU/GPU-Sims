// boid_render.vert.wgsl — instanced vertex shader for boid+predator rendering.
//
// Each instance reads its slot from the entity buffer (post-integrate state).
// The local-to-world transform is constructed from the instance's velocity:
//   forward_axis = normalize(velocity), or (0, 0, 1) if velocity is near-zero
//   up_seed     = (0, 1, 0), or (1, 0, 0) if forward_axis ‖ world_up
//   right_axis  = normalize(cross(forward_axis, up_seed))
//   up_axis     = cross(right_axis, forward_axis)
//
// Mesh: 4-tri pyramid with apex at local +Z (boid's "front") and three base
// vertices in the local XY plane at z = -1. 12 vertex indices total
// (= 4 triangles × 3 vertices).
//
// === Gram-Schmidt singularity fallback (architect-2 callout #5) ===
// World coordinates: Y-up, right-handed. world_up = (0, 1, 0).
// world_forward (the fallback up-reference) = (0, 0, -1) — orthogonal to
// world_up so the cross-product is well-defined for any forward_axis that
// happens to be parallel to world_up.
// Singularity 1: forward_axis ‖ world_up — handled by the select() below.
// Singularity 2: forward_axis ‖ world_forward — geometrically impossible
//   given the select() guarantees (forward_axis is either non-parallel to
//   world_up, in which case up_seed = world_up and forward × world_up is
//   well-defined, OR forward_axis is parallel to world_up, in which case
//   up_seed = world_forward and forward × world_forward is well-defined
//   because forward_axis = ±world_up which is orthogonal to world_forward).
// No third case to worry about.

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

struct CameraUniforms {
    viewProj:  mat4x4<f32>,
    cameraPos: vec3<f32>,
    _pad0:     f32,
    lightDir:  vec3<f32>,
    ambient:   f32,
}

@group(0) @binding(0) var<uniform>      camera:   CameraUniforms;
@group(0) @binding(1) var<uniform>      params:   Params;
@group(0) @binding(2) var<storage, read> entities: array<Entity>;

struct VertexOut {
    @builtin(position) clip_position: vec4<f32>,
    @location(0)       world_normal:  vec3<f32>,
    @location(1) @interpolate(flat) species: u32,
}

// 4-tri pyramid: apex at +Z, three base verts in -Z plane forming an equilateral triangle.
// Mesh vertices in mesh-local space (unit scale; multiplied by boidScale at instance time).
fn local_vertex(idx: u32) -> vec3<f32> {
    let r = 0.42;       // base radius
    let h = 0.85;       // apex height (z component)
    // Base verts at angles 90°, 210°, 330° around the -Z plane.
    let a0 = vec3<f32>(0.0,                r,                 -h * 0.4);
    let a1 = vec3<f32>(-r * 0.866,         -r * 0.5,          -h * 0.4);
    let a2 = vec3<f32>( r * 0.866,         -r * 0.5,          -h * 0.4);
    let apex = vec3<f32>(0.0, 0.0, h);

    // 4 triangles (12 verts):
    //   tri 0: apex, a0, a1
    //   tri 1: apex, a1, a2
    //   tri 2: apex, a2, a0
    //   tri 3: a2,   a1, a0   (base, back-facing the apex)
    switch (idx) {
        case 0u:  { return apex; }
        case 1u:  { return a0; }
        case 2u:  { return a1; }
        case 3u:  { return apex; }
        case 4u:  { return a1; }
        case 5u:  { return a2; }
        case 6u:  { return apex; }
        case 7u:  { return a2; }
        case 8u:  { return a0; }
        case 9u:  { return a2; }
        case 10u: { return a1; }
        default:  { return a0; }
    }
}

@vertex
fn vs_main(
    @builtin(vertex_index)   vidx: u32,
    @builtin(instance_index) iidx: u32,
) -> VertexOut {
    let inst = entities[iidx];

    // Velocity-derived orientation with Gram-Schmidt singularity fallback.
    let v_norm = select(
        vec3<f32>(0.0, 0.0, 1.0),
        normalize(inst.velocity),
        length(inst.velocity) > 1e-3,
    );
    let world_up      = vec3<f32>(0.0, 1.0, 0.0);
    let world_forward = vec3<f32>(0.0, 0.0, -1.0);
    let up_seed = select(world_up, world_forward, abs(dot(v_norm, world_up)) > 0.99);
    let right_axis = normalize(cross(v_norm, up_seed));
    let up_axis    = cross(right_axis, v_norm);

    // Per-species scale (predators 2× boids).
    let scale = select(params.boidScale, params.boidScale * 2.0, inst.species == 1u);

    // Apply local→world transform.
    let local = local_vertex(vidx);
    let world = inst.pos
              + right_axis * (local.x * scale)
              + up_axis    * (local.y * scale)
              + v_norm     * (local.z * scale);

    // Approximate normal: same direction as the local vertex, transformed by
    // the basis. For the apex (local.z > 0) this points forward; for base
    // verts it points slightly back-and-out. Adequate for diffuse lighting
    // at the visual scale where each boid is 1–4 pixels.
    let n_local = normalize(local);
    let world_normal = right_axis * n_local.x + up_axis * n_local.y + v_norm * n_local.z;

    var out: VertexOut;
    out.clip_position = camera.viewProj * vec4<f32>(world, 1.0);
    out.world_normal  = world_normal;
    out.species       = inst.species;
    return out;
}
