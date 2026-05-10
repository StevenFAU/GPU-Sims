// Reaction-Diffusion 2D brush stamping kernel.
// Applied once per rendered frame (NOT per substep) when LMB is held.
// Splats v material around the cursor with a soft distance falloff.

struct BrushParams {
    gridSize:    vec2<u32>,
    centerCell:  vec2<f32>,    // brush center in cell coords
    radiusCells: f32,
    strength:    f32,
    _pad:        vec2<f32>,
};

@group(0) @binding(0) var<uniform> brush: BrushParams;
@group(0) @binding(1) var srcTex: texture_2d<f32>;
@group(0) @binding(2) var srcSampler: sampler;
@group(0) @binding(3) var dstTex: texture_storage_2d<rg32float, write>;

@compute @workgroup_size(16, 16, 1)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
    if (gid.x >= brush.gridSize.x || gid.y >= brush.gridSize.y) {
        return;
    }

    let invSize  = 1.0 / vec2<f32>(brush.gridSize);
    let centerUV = (vec2<f32>(gid.xy) + 0.5) * invSize;
    let cur      = textureSampleLevel(srcTex, srcSampler, centerUV, 0.0).xy;

    let d        = length(vec2<f32>(gid.xy) - brush.centerCell);
    let falloff  = max(0.0, 1.0 - d / max(brush.radiusCells, 1.0));
    let amount   = falloff * brush.strength;

    let nu = clamp(cur.x - 0.5 * amount, 0.0, 1.0);
    let nv = clamp(cur.y + 0.5 * amount, 0.0, 1.0);

    textureStore(dstTex, vec2<i32>(gid.xy), vec4<f32>(nu, nv, 0.0, 0.0));
}
