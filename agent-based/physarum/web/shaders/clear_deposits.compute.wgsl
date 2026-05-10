// Zero the three species deposit buffers each frame, before the agent kernel
// adds new contributions and the diffuse-decay pass folds them back into the
// trail texture.

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
@group(0) @binding(1) var<storage, read_write> deposits0: array<atomic<u32>>;
@group(0) @binding(2) var<storage, read_write> deposits1: array<atomic<u32>>;
@group(0) @binding(3) var<storage, read_write> deposits2: array<atomic<u32>>;

// Workgroup size 256 (not 64): the 2048² grid clear dispatches
// ceil(2048*2048/64) = 65,536 workgroups in X, exceeding the baseline
// maxComputeWorkgroupsPerDimension (65,535) by one. 256 brings it to 16,384.
@compute @workgroup_size(256, 1, 1)
fn cs_main(@builtin(global_invocation_id) gid: vec3<u32>) {
    let total = params.gridSize * params.gridSize;
    if (gid.x >= total) { return; }
    atomicStore(&deposits0[gid.x], 0u);
    atomicStore(&deposits1[gid.x], 0u);
    atomicStore(&deposits2[gid.x], 0u);
}
