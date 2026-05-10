// Trail diffuse + decay. 2D dispatch over the grid.
// 9-tap kernel (4 cardinal × 1.0, 4 diagonal × 0.7, center × 4.0) / 10.8.
// Decay: multiplicative (1 - decayRate) per frame.
//
// Reads the deposit buffers as non-atomic array<u32> — safe because compute
// passes are serialized within a command encoder; the agent-move and
// pin-deposit passes have already completed when this dispatches.

struct Params {
    gridSize:        u32,
    agentCount:      u32,
    iteration:       u32,
    depositScale:    u32,
    senseDistance:   f32,
    senseAngle:      f32,
    turnAngle:       f32,
    stepSize:        f32,
    decayRate:       f32,
    diffuseWeight:   f32,
    depositAmount:   f32,
    repulsionStrength: f32,
    simSpeed:        f32,
    pinCount:        u32,
    pinIntensity:    f32,
    pinRadius:       f32,
}

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var trailPrev: texture_2d<f32>;
@group(0) @binding(2) var trailSampler: sampler;
@group(0) @binding(3) var<storage, read> deposits0: array<u32>;
@group(0) @binding(4) var<storage, read> deposits1: array<u32>;
@group(0) @binding(5) var<storage, read> deposits2: array<u32>;
@group(0) @binding(6) var trailNext: texture_storage_2d<rgba16float, write>;

@compute @workgroup_size(16, 16, 1)
fn cs_main(@builtin(global_invocation_id) gid: vec3<u32>) {
    let g = params.gridSize;
    if (gid.x >= g || gid.y >= g) { return; }

    let gf = f32(g);
    let invG = 1.0 / gf;
    let center = vec2<f32>(f32(gid.x) + 0.5, f32(gid.y) + 0.5) * invG;
    let off = invG;

    let c00 = textureSampleLevel(trailPrev, trailSampler, center,                            0.0).rgb;
    let cN  = textureSampleLevel(trailPrev, trailSampler, center + vec2<f32>( 0.0,  off),     0.0).rgb;
    let cS  = textureSampleLevel(trailPrev, trailSampler, center + vec2<f32>( 0.0, -off),     0.0).rgb;
    let cE  = textureSampleLevel(trailPrev, trailSampler, center + vec2<f32>( off,  0.0),     0.0).rgb;
    let cW  = textureSampleLevel(trailPrev, trailSampler, center + vec2<f32>(-off,  0.0),     0.0).rgb;
    let cNE = textureSampleLevel(trailPrev, trailSampler, center + vec2<f32>( off,  off),     0.0).rgb;
    let cNW = textureSampleLevel(trailPrev, trailSampler, center + vec2<f32>(-off,  off),     0.0).rgb;
    let cSE = textureSampleLevel(trailPrev, trailSampler, center + vec2<f32>( off, -off),     0.0).rgb;
    let cSW = textureSampleLevel(trailPrev, trailSampler, center + vec2<f32>(-off, -off),     0.0).rgb;

    // 9-tap kernel weights: 4 (center) + 4×1.0 (cardinal) + 4×0.7 (diagonal) = 10.8.
    let blurred = (4.0 * c00
                 + 1.0 * (cN + cS + cE + cW)
                 + 0.7 * (cNE + cNW + cSE + cSW)) / 10.8;

    let diffused = mix(c00, blurred, params.diffuseWeight);

    let idx = gid.y * g + gid.x;
    let s = 1.0 / f32(params.depositScale);
    let dep = vec3<f32>(
        f32(deposits0[idx]) * s,
        f32(deposits1[idx]) * s,
        f32(deposits2[idx]) * s,
    );

    let result = (diffused + dep) * (1.0 - params.decayRate);

    textureStore(trailNext, vec2<i32>(i32(gid.x), i32(gid.y)),
                 vec4<f32>(result, 0.0));
}
