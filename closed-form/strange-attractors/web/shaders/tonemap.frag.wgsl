// Composite HDR accumulator + bloom, Reinhard tonemap, output to swapchain.
// Vertex shader is fullscreen.vert.wgsl.

@group(0) @binding(0) var srcSampler: sampler;
@group(0) @binding(1) var hdrTexture: texture_2d<f32>;
@group(0) @binding(2) var bloomTexture: texture_2d<f32>;

struct TonemapUniforms {
    bloomIntensity: f32,
    exposure: f32,
    _pad0: f32,
    _pad1: f32,
};
@group(0) @binding(3) var<uniform> tu: TonemapUniforms;

@fragment
fn fs_main(@location(0) uv: vec2<f32>) -> @location(0) vec4<f32> {
    let hdr = textureSample(hdrTexture, srcSampler, uv).rgb;
    let bloom = textureSample(bloomTexture, srcSampler, uv).rgb;
    let combined = hdr + bloom * tu.bloomIntensity;
    let exposed = combined * tu.exposure;
    // Reinhard.
    let mapped = exposed / (vec3<f32>(1.0) + exposed);
    return vec4<f32>(mapped, 1.0);
}
