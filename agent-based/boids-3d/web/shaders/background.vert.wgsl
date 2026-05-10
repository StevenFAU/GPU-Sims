// background.vert.wgsl — fullscreen triangle with UV output.
//
// UV layout: (0, 0) at canvas top-left, (1, 1) at canvas bottom-right.
// Three vertices form a single oversized triangle that covers the full
// canvas area; the fragment shader's UV.y selects the gradient mix factor.
//
// Coordinates:
//   v0: clip (-1,  1), uv (0, 0)   — top-left
//   v1: clip ( 3,  1), uv (2, 0)   — far right (extends past canvas)
//   v2: clip (-1, -3), uv (0, 2)   — far bottom (extends past canvas)

struct VertexOut {
    @builtin(position) clip_position: vec4<f32>,
    @location(0)       uv:            vec2<f32>,
}

@vertex
fn vs_main(@builtin(vertex_index) vidx: u32) -> VertexOut {
    var pos: vec2<f32>;
    var uv:  vec2<f32>;
    switch (vidx) {
        case 0u: { pos = vec2<f32>(-1.0,  1.0); uv = vec2<f32>(0.0, 0.0); }
        case 1u: { pos = vec2<f32>( 3.0,  1.0); uv = vec2<f32>(2.0, 0.0); }
        default: { pos = vec2<f32>(-1.0, -3.0); uv = vec2<f32>(0.0, 2.0); }
    }
    var out: VertexOut;
    out.clip_position = vec4<f32>(pos, 0.999, 1.0);   // depth=0.999 so the wireframe (depth-tested 'less') draws over
    out.uv            = uv;
    return out;
}
