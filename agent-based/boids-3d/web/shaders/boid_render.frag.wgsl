// boid_render.frag.wgsl — fragment shader for boid+predator instances.
//
// Per-species color selection (boid vs predator) plus simple Lambert
// diffuse + ambient lighting. Light direction in world space.

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

struct CameraUniforms {
    viewProj:  mat4x4<f32>,
    cameraPos: vec3<f32>,
    _pad0:     f32,
    lightDir:  vec3<f32>,
    ambient:   f32,
}

@group(0) @binding(0) var<uniform> camera: CameraUniforms;
@group(0) @binding(1) var<uniform> params: Params;

struct FragInput {
    @location(0) world_normal: vec3<f32>,
    @location(1) @interpolate(flat) species: u32,
}

@fragment
fn fs_main(in: FragInput) -> @location(0) vec4<f32> {
    let base_color = select(params.boidColor, params.predatorColor, in.species == 1u);
    let n = normalize(in.world_normal);
    let l = normalize(camera.lightDir);
    let lambert = max(dot(n, l), 0.0);
    let lit = base_color * (camera.ambient + (1.0 - camera.ambient) * lambert);
    return vec4<f32>(lit, 1.0);
}
