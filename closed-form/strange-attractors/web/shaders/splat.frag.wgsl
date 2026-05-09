// Splat fragment shader. Anti-aliased point sprite via smoothstep on
// distance from quad center. Color from velocity-magnitude LUT lookup.
// Output is additively blended into the rgba16float accumulator.

struct RenderUniforms {
    viewProj: mat4x4<f32>,
    viewportSize: vec2<f32>,
    pointSizePx: f32,
    depthAttenK: f32,
    colorSpeedScale: f32,
    colorExponent: f32,
    colormapIndex: f32,
    _pad: f32,
};

@group(0) @binding(1) var<uniform> ru: RenderUniforms;
@group(0) @binding(2) var lutSampler: sampler;
@group(0) @binding(3) var lutTexture: texture_2d<f32>;

@fragment
fn fs_main(
    @location(0) localUv: vec2<f32>,
    @location(1) speed: f32,
    @location(2) viewDepth: f32,
    @location(3) @interpolate(flat) colormapIndex: u32,
) -> @location(0) vec4<f32> {
    let d = length(localUv);
    if (d > 1.0) {
        discard;
    }
    // smoothstep edge: opaque inside 0.6, ramp out to 1.0
    let aa = 1.0 - smoothstep(0.6, 1.0, d);

    // Normalized speed -> LUT index.
    let t = clamp(speed / max(ru.colorSpeedScale, 0.0001), 0.0, 1.0);
    let tShaped = pow(t, ru.colorExponent);

    // Sample one of four LUT rows; rows are at v = (idx + 0.5) / 4.
    let v = (f32(colormapIndex) + 0.5) / 4.0;
    let lut = textureSample(lutTexture, lutSampler, vec2<f32>(tShaped, v));

    // Depth attenuation.
    let depthAtten = 1.0 / (1.0 + viewDepth * ru.depthAttenK);

    // Output additively. Alpha controls energy contribution per splat.
    let energy = aa * depthAtten;
    return vec4<f32>(lut.rgb * energy, energy);
}
