// 9-tap separable Gaussian blur. Direction is a uniform: (1,0) for
// horizontal pass, (0,1) for vertical.

@group(0) @binding(0) var srcSampler: sampler;
@group(0) @binding(1) var srcTexture: texture_2d<f32>;
@group(0) @binding(2) var dstTexture: texture_storage_2d<rgba16float, write>;

struct BlurUniforms {
    direction: vec2<f32>,   // (1,0) or (0,1)
    _pad0: f32,
    _pad1: f32,
};
@group(0) @binding(3) var<uniform> bu: BlurUniforms;

const WEIGHTS: array<f32, 5> = array<f32, 5>(
    0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216
);

@compute @workgroup_size(8, 8, 1)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
    let dstSize = textureDimensions(dstTexture);
    if (gid.x >= dstSize.x || gid.y >= dstSize.y) {
        return;
    }
    let texel = 1.0 / vec2<f32>(dstSize);
    let uv = (vec2<f32>(gid.xy) + 0.5) * texel;

    var acc = textureSampleLevel(srcTexture, srcSampler, uv, 0.0).rgb * WEIGHTS[0];
    for (var i: i32 = 1; i < 5; i = i + 1) {
        let off = bu.direction * (f32(i) * texel);
        acc = acc + textureSampleLevel(srcTexture, srcSampler, uv + off, 0.0).rgb * WEIGHTS[i];
        acc = acc + textureSampleLevel(srcTexture, srcSampler, uv - off, 0.0).rgb * WEIGHTS[i];
    }
    textureStore(dstTexture, vec2<i32>(gid.xy), vec4<f32>(acc, 1.0));
}
