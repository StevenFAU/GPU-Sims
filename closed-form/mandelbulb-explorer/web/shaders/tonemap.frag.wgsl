// Tonemap fragment shader. Samples the HDR offscreen RT, applies exposure,
// applies Reinhard tonemap, writes to the swap-chain image.

struct TonemapUniforms {
    // x = exposure, y = unused, z = unused, w = unused
    params: vec4<f32>,
};

@group(0) @binding(0) var<uniform> u:    TonemapUniforms;
@group(0) @binding(1) var          samp: sampler;
@group(0) @binding(2) var          src:  texture_2d<f32>;

@fragment
fn fs_main(@location(0) uv: vec2<f32>) -> @location(0) vec4<f32> {
    let hdr = textureSample(src, samp, uv).rgb;
    let exposed = hdr * u.params.x;
    let mapped = exposed / (vec3<f32>(1.0) + exposed);
    return vec4<f32>(mapped, 1.0);
}
