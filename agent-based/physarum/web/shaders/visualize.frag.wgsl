// RGB-per-species direct-blit visualization.
// Trail RGB → tinted by per-species colors → exposed → clamped → output.
// Plus 1-pixel white outlines around active food pins.

struct VizParams {
    canvasSize:      vec2<f32>,
    gridSize:        u32,
    pinCount:        u32,
    pinRadius:       f32,
    trailExposure:   f32,
    _pad0:           vec2<f32>,
    colorSpecies0:   vec3<f32>,
    _pad1:           f32,
    colorSpecies1:   vec3<f32>,
    _pad2:           f32,
    colorSpecies2:   vec3<f32>,
    _pad3:           f32,
}

struct FoodPin {
    pos:         vec2<f32>,
    intensity:   f32,
    speciesMask: u32,
}

@group(0) @binding(0) var<uniform> params: VizParams;
@group(0) @binding(1) var trailLatest: texture_2d<f32>;
@group(0) @binding(2) var trailSampler: sampler;
@group(0) @binding(3) var<storage, read> pins: array<FoodPin, 32>;

@fragment
fn fs_main(@location(0) uv: vec2<f32>) -> @location(0) vec4<f32> {
    let trail = textureSample(trailLatest, trailSampler, uv).rgb;
    let tinted = trail.r * params.colorSpecies0
               + trail.g * params.colorSpecies1
               + trail.b * params.colorSpecies2;
    let exposed = tinted * params.trailExposure;
    var color = min(exposed, vec3<f32>(1.0));

    // Pin outlines: 1-cell-wide white ring at the pin's grid radius.
    // Geometry done in cell space so the ring stays circular on
    // non-square canvases.
    if (params.pinCount > 0u) {
        let curCell = uv * f32(params.gridSize);
        for (var p = 0u; p < params.pinCount; p = p + 1u) {
            let d = distance(curCell, pins[p].pos);
            if (abs(d - params.pinRadius) < 0.5) {
                color = vec3<f32>(1.0, 1.0, 1.0);
            }
        }
    }

    return vec4<f32>(color, 1.0);
}
