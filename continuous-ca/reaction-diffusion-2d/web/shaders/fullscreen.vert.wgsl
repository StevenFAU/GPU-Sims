// Single-triangle fullscreen vertex shader (no vertex buffer).
// UV layout matches strange-attractors and mandelbulb-explorer:
//   uv = (0, 0) at canvas TOP, uv = (1, 1) at canvas BOTTOM.
// This matches WebGPU's framebuffer-y origin so render-then-textureSample
// round-trips don't visually flip.

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
