// Writes a time-varying gradient into a 2D rgba8unorm storage texture.

@group(0) @binding(0) var outImage: texture_storage_2d<rgba8unorm, write>;

struct Push {
    resolution: vec2<f32>,
    time: f32,
    _pad: f32,
};
@group(0) @binding(1) var<uniform> push: Push;

@compute @workgroup_size(16, 16, 1)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
    let p = vec2<i32>(gid.xy);
    if (p.x >= i32(push.resolution.x) || p.y >= i32(push.resolution.y)) {
        return;
    }
    let uv = (vec2<f32>(p) + 0.5) / push.resolution;
    let t  = push.time;

    let r = 0.5 + 0.5 * sin(uv.x * 6.28318 + t * 0.6);
    let g = 0.5 + 0.5 * sin(uv.y * 6.28318 + t * 0.4 + 2.094);
    let b = 0.5 + 0.5 * sin((uv.x + uv.y) * 6.28318 + t * 0.5 + 4.188);

    textureStore(outImage, p, vec4<f32>(r, g, b, 1.0));
}
