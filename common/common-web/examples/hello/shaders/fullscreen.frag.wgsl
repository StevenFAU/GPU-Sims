@group(0) @binding(0) var srcSampler: sampler;
@group(0) @binding(1) var srcTexture: texture_2d<f32>;

@fragment
fn fs_main(@location(0) uv: vec2<f32>) -> @location(0) vec4<f32> {
    return textureSample(srcTexture, srcSampler, uv);
}
