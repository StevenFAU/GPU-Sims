// leader_render.vert.wgsl — instanced octahedron mesh for leader markers.
//
// 8 triangles (24 vertex indices) hardcoded in mesh-local space. Each
// instance places a leader-marker at the leader's world position with
// fixed scale 0.4 units. Soft-white color from params.leaderColor.

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

struct Leader { position: vec3<f32>, strength: f32 }

struct CameraUniforms {
    viewProj:  mat4x4<f32>,
    cameraPos: vec3<f32>,
    _pad0:     f32,
    lightDir:  vec3<f32>,
    ambient:   f32,
}

@group(0) @binding(0) var<uniform>       camera:  CameraUniforms;
@group(0) @binding(1) var<uniform>       params:  Params;
@group(0) @binding(2) var<storage, read> leaders: array<Leader>;

const LEADER_SCALE: f32 = 0.4;

struct VertexOut {
    @builtin(position) clip_position: vec4<f32>,
    @location(0)       world_normal:  vec3<f32>,
}

// Octahedron (8 triangles, 24 verts). Distinct visual character from the
// boid/predator pyramid mesh — reads as a "diamond marker" at the leader's
// world position. No icosphere subdivision (would require more verts than
// the visual benefit warrants at the scale where leaders are ~10 px tall).
//
// === Winding (architect-2 verified) ===
// All triangles wound CCW from outside, so cross-product normals point
// AWAY from the origin (verified: normal·centroid = +1.0 for all 8 tris).
// Compatible with the leader pipeline's cullMode: 'back' + frontFace: 'ccw'
// (default). An earlier draft had inward winding which culled every triangle.
fn icosphere_vertex(idx: u32) -> vec3<f32> {
    let r = 1.0;
    let xp = vec3<f32>( r,  0.0,  0.0);  let xn = vec3<f32>(-r,  0.0,  0.0);
    let yp = vec3<f32>( 0.0,  r,  0.0);  let yn = vec3<f32>( 0.0, -r,  0.0);
    let zp = vec3<f32>( 0.0,  0.0,  r);  let zn = vec3<f32>( 0.0,  0.0, -r);
    // 8 triangular faces of the octahedron (one per octant of the sphere):
    //   top    : (yp, zp, xp), (yp, xn, zp), (yp, zn, xn), (yp, xp, zn)
    //   bottom : (yn, xp, zp), (yn, zp, xn), (yn, xn, zn), (yn, zn, xp)
    // (Each tri swapped from the natural-octant order to flip CW→CCW from outside.)
    switch (idx) {
        case  0u: { return yp; } case  1u: { return zp; } case  2u: { return xp; }
        case  3u: { return yp; } case  4u: { return xn; } case  5u: { return zp; }
        case  6u: { return yp; } case  7u: { return zn; } case  8u: { return xn; }
        case  9u: { return yp; } case 10u: { return xp; } case 11u: { return zn; }
        case 12u: { return yn; } case 13u: { return xp; } case 14u: { return zp; }
        case 15u: { return yn; } case 16u: { return zp; } case 17u: { return xn; }
        case 18u: { return yn; } case 19u: { return xn; } case 20u: { return zn; }
        case 21u: { return yn; } case 22u: { return zn; } default: { return xp; }
    }
}

@vertex
fn vs_main(
    @builtin(vertex_index)   vidx: u32,
    @builtin(instance_index) iidx: u32,
) -> VertexOut {
    let lead = leaders[iidx];
    let local = icosphere_vertex(vidx);
    let world = lead.position + local * LEADER_SCALE;
    var out: VertexOut;
    out.clip_position = camera.viewProj * vec4<f32>(world, 1.0);
    out.world_normal  = normalize(local);   // octahedron verts are already unit-distance from origin
    return out;
}
