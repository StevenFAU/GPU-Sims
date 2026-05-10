// wireframe.vert.wgsl — 12-edge box wireframe.
//
// 24 vertices hardcoded in clip-space-via-world: 2 verts per edge × 12 edges.
// Drawn as topology: 'line-list' so each pair of consecutive vertices forms
// one edge.
//
// Box: [-h, h]^3 where h = 16.0 (matches BOX_HALF_EXTENT in main.ts).
// If BOX_HALF_EXTENT ever changes, update this constant in lockstep.

struct CameraUniforms {
    viewProj:  mat4x4<f32>,
    cameraPos: vec3<f32>,
    _pad0:     f32,
    lightDir:  vec3<f32>,
    ambient:   f32,
}

@group(0) @binding(0) var<uniform> camera: CameraUniforms;

const H: f32 = 16.0;     // KEEP IN SYNC WITH main.ts BOX_HALF_EXTENT

fn edge_vertex(idx: u32) -> vec3<f32> {
    // 8 corner labels:
    //   c0 = (-h, -h, -h),  c1 = (+h, -h, -h),  c2 = (+h, +h, -h),  c3 = (-h, +h, -h)
    //   c4 = (-h, -h, +h),  c5 = (+h, -h, +h),  c6 = (+h, +h, +h),  c7 = (-h, +h, +h)
    // 12 edges (pairs):
    //    bottom face: (c0,c1) (c1,c2) (c2,c3) (c3,c0)
    //    top face:    (c4,c5) (c5,c6) (c6,c7) (c7,c4)
    //    verticals:   (c0,c4) (c1,c5) (c2,c6) (c3,c7)
    let c0 = vec3<f32>(-H, -H, -H);  let c1 = vec3<f32>( H, -H, -H);
    let c2 = vec3<f32>( H,  H, -H);  let c3 = vec3<f32>(-H,  H, -H);
    let c4 = vec3<f32>(-H, -H,  H);  let c5 = vec3<f32>( H, -H,  H);
    let c6 = vec3<f32>( H,  H,  H);  let c7 = vec3<f32>(-H,  H,  H);
    switch (idx) {
        case  0u: { return c0; } case  1u: { return c1; }
        case  2u: { return c1; } case  3u: { return c2; }
        case  4u: { return c2; } case  5u: { return c3; }
        case  6u: { return c3; } case  7u: { return c0; }
        case  8u: { return c4; } case  9u: { return c5; }
        case 10u: { return c5; } case 11u: { return c6; }
        case 12u: { return c6; } case 13u: { return c7; }
        case 14u: { return c7; } case 15u: { return c4; }
        case 16u: { return c0; } case 17u: { return c4; }
        case 18u: { return c1; } case 19u: { return c5; }
        case 20u: { return c2; } case 21u: { return c6; }
        case 22u: { return c3; } default:  { return c7; }
    }
}

@vertex
fn vs_main(@builtin(vertex_index) vidx: u32) -> @builtin(position) vec4<f32> {
    let world = edge_vertex(vidx);
    return camera.viewProj * vec4<f32>(world, 1.0);
}
