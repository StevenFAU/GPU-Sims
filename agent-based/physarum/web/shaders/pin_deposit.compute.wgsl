// Persistent food-pin deposit pass. 2D dispatch over the grid.
// For each cell, accumulate contributions from all active pins within radius
// and atomicAdd to the species deposit buffers.
//
// Atomics are required (not just simple stores) because the agent-move pass
// has already written this frame's deposit increments into the same buffers.

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

struct FoodPin {
    pos:         vec2<f32>,
    intensity:   f32,
    speciesMask: u32,
}

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read>       pins: array<FoodPin, 32>;
@group(0) @binding(2) var<storage, read_write> deposits0: array<atomic<u32>>;
@group(0) @binding(3) var<storage, read_write> deposits1: array<atomic<u32>>;
@group(0) @binding(4) var<storage, read_write> deposits2: array<atomic<u32>>;

@compute @workgroup_size(8, 8, 1)
fn cs_main(@builtin(global_invocation_id) gid: vec3<u32>) {
    let g = params.gridSize;
    if (gid.x >= g || gid.y >= g) { return; }
    if (params.pinCount == 0u) { return; }

    let cellPos = vec2<f32>(f32(gid.x) + 0.5, f32(gid.y) + 0.5);
    var contribR: f32 = 0.0;
    var contribG: f32 = 0.0;
    var contribB: f32 = 0.0;

    for (var p = 0u; p < params.pinCount; p = p + 1u) {
        let pin = pins[p];
        let d = distance(cellPos, pin.pos);
        if (d < params.pinRadius) {
            let falloff = 1.0 - (d / params.pinRadius);
            let amount = pin.intensity * falloff * falloff;
            if ((pin.speciesMask & 1u) != 0u) { contribR = contribR + amount; }
            if ((pin.speciesMask & 2u) != 0u) { contribG = contribG + amount; }
            if ((pin.speciesMask & 4u) != 0u) { contribB = contribB + amount; }
        }
    }

    let idx = gid.y * g + gid.x;
    let s = f32(params.depositScale);
    let s0 = u32(round(contribR * s));
    let s1 = u32(round(contribG * s));
    let s2 = u32(round(contribB * s));
    if (s0 > 0u) { atomicAdd(&deposits0[idx], s0); }
    if (s1 > 0u) { atomicAdd(&deposits1[idx], s1); }
    if (s2 > 0u) { atomicAdd(&deposits2[idx], s2); }
}
