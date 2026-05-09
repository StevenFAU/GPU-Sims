// Fragment shader for the decay pass. Samples the previous accumulator
// and writes alpha * sample to the next accumulator.
// Vertex shader is the shared fullscreen.vert.wgsl.

@group(0) @binding(0) var srcSampler: sampler;
@group(0) @binding(1) var srcTexture: texture_2d<f32>;

struct DecayUniforms {
    alpha: f32,
    _pad0: f32,
    _pad1: f32,
    _pad2: f32,
};
@group(0) @binding(2) var<uniform> decay: DecayUniforms;

@fragment
fn fs_main(@location(0) uv: vec2<f32>) -> @location(0) vec4<f32> {
    let s = textureSample(srcTexture, srcSampler, uv);
    return s * decay.alpha;
}
