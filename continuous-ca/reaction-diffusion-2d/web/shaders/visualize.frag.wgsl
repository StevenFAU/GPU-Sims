// Reaction-Diffusion 2D visualization.
// Reads the latest state texture, picks the V channel, manually bilinearly
// interpolates across cells (rg32float is not in WebGPU's baseline
// filterable list), maps through a colormap LUT, outputs to the canvas.

struct VizParams {
    gridSize:  vec2<f32>,
    minMax:    vec2<f32>,    // contrast-stretch range for V
    colormap:  f32,          // 0..3 = magma, inferno, viridis, hsv
    _pad0:     f32,
    _pad1:     vec2<f32>,
};

@group(0) @binding(0) var<uniform> viz:        VizParams;
@group(0) @binding(1) var stateTex:            texture_2d<f32>;
@group(0) @binding(2) var lutTex:              texture_2d<f32>;
@group(0) @binding(3) var lutSampler:          sampler;

fn wrapI(v: i32, n: i32) -> i32 {
    return ((v % n) + n) % n;
}

fn sampleManualBilinear(coord: vec2<f32>) -> vec2<f32> {
    let x  = coord.x - 0.5;
    let y  = coord.y - 0.5;
    let x0 = floor(x);
    let y0 = floor(y);
    let fx = x - x0;
    let fy = y - y0;

    let nx = i32(viz.gridSize.x);
    let ny = i32(viz.gridSize.y);

    let i00 = vec2<i32>(wrapI(i32(x0),     nx), wrapI(i32(y0),     ny));
    let i10 = vec2<i32>(wrapI(i32(x0) + 1, nx), wrapI(i32(y0),     ny));
    let i01 = vec2<i32>(wrapI(i32(x0),     nx), wrapI(i32(y0) + 1, ny));
    let i11 = vec2<i32>(wrapI(i32(x0) + 1, nx), wrapI(i32(y0) + 1, ny));

    let c00 = textureLoad(stateTex, i00, 0).xy;
    let c10 = textureLoad(stateTex, i10, 0).xy;
    let c01 = textureLoad(stateTex, i01, 0).xy;
    let c11 = textureLoad(stateTex, i11, 0).xy;

    let cx0 = mix(c00, c10, fx);
    let cx1 = mix(c01, c11, fx);
    return mix(cx0, cx1, fy);
}

@fragment
fn fs_main(@location(0) uv: vec2<f32>) -> @location(0) vec4<f32> {
    let clamped = clamp(uv, vec2<f32>(0.0), vec2<f32>(1.0));
    let cellPos = clamped * viz.gridSize;
    let cv      = sampleManualBilinear(cellPos);

    let lo  = viz.minMax.x;
    let hi  = viz.minMax.y;
    let t   = clamp((cv.y - lo) / max(hi - lo, 1e-6), 0.0, 1.0);
    let row = (viz.colormap + 0.5) / 4.0;

    let rgb = textureSampleLevel(lutTex, lutSampler, vec2<f32>(t, row), 0.0).rgb;
    return vec4<f32>(rgb, 1.0);
}
