// Fullscreen triangle. UV layout matches strange-attractors / mandelbulb / rd-2d:
// uv = (0, 0) at canvas TOP, (1, 1) at canvas BOTTOM-RIGHT.
// Matches WebGPU's framebuffer-y origin and texture-sample convention.

struct VsOut {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
}

@vertex
fn vs_main(@builtin(vertex_index) vid: u32) -> VsOut {
    var out: VsOut;
    let pos = array<vec2<f32>, 3>(
        vec2<f32>(-1.0,  3.0),
        vec2<f32>(-1.0, -1.0),
        vec2<f32>( 3.0, -1.0),
    );
    let uvs = array<vec2<f32>, 3>(
        vec2<f32>(0.0, -1.0),
        vec2<f32>(0.0,  1.0),
        vec2<f32>(2.0,  1.0),
    );
    out.position = vec4<f32>(pos[vid], 0.0, 1.0);
    out.uv = uvs[vid];
    return out;
}
