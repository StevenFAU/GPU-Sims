// Fullscreen-triangle vertex shader. Three vertices cover the screen in
// clip space; UV runs 0..2 (the off-screen vertex sits at (3,-1) in NDC,
// which when interpolated gives uv = (0,0)..(1,1) in the visible region).
// No vertex buffer.

struct VsOut {
    @builtin(position) pos: vec4<f32>,
    @location(0)       uv:  vec2<f32>,
};

@vertex
fn vs_main(@builtin(vertex_index) vid: u32) -> VsOut {
    var positions = array<vec2<f32>, 3>(
        vec2<f32>(-1.0, -1.0),
        vec2<f32>( 3.0, -1.0),
        vec2<f32>(-1.0,  3.0),
    );
    var uvs = array<vec2<f32>, 3>(
        vec2<f32>(0.0, 1.0),
        vec2<f32>(2.0, 1.0),
        vec2<f32>(0.0, -1.0),
    );

    var out: VsOut;
    out.pos = vec4<f32>(positions[vid], 0.0, 1.0);
    out.uv  = uvs[vid];
    return out;
}
