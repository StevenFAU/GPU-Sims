// background.frag.wgsl — vertical gradient.
// Top color (uv.y = 0):    vec3(0.05, 0.06, 0.08)  — dark blue-gray
// Bottom color (uv.y = 1): vec3(0.02, 0.025, 0.04) — near-black
// Linear interpolation in uv.y.

struct FragInput {
    @location(0) uv: vec2<f32>,
}

@fragment
fn fs_main(in: FragInput) -> @location(0) vec4<f32> {
    let top    = vec3<f32>(0.05, 0.06,  0.08);
    let bottom = vec3<f32>(0.02, 0.025, 0.04);
    let t = clamp(in.uv.y, 0.0, 1.0);
    let color = mix(top, bottom, t);
    return vec4<f32>(color, 1.0);
}
