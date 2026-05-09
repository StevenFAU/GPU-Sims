// Instanced point-quad vertex shader. Six vertices per particle form two
// triangles for a screen-aligned quad billboarded around the particle's
// world position.
//
// Bindings:
//   group 0:
//     0: positions storage buffer (vec4<f32>: xyz position + w speed)
//     1: render uniforms (camera VP + viewport size + point size)

struct RenderUniforms {
    viewProj: mat4x4<f32>,
    viewportSize: vec2<f32>,    // pixels
    pointSizePx: f32,
    depthAttenK: f32,
    colorSpeedScale: f32,
    colorExponent: f32,
    colormapIndex: f32,         // 0..3 -> magma/inferno/viridis/hsv (encoded as float for alignment)
    _pad: f32,
};

@group(0) @binding(0) var<storage, read> positions: array<vec4<f32>>;
@group(0) @binding(1) var<uniform> ru: RenderUniforms;

struct VsOut {
    @builtin(position) pos: vec4<f32>,
    @location(0) localUv: vec2<f32>,    // -1..1 across the quad
    @location(1) speed: f32,
    @location(2) viewDepth: f32,
    @location(3) @interpolate(flat) colormapIndex: u32,
};

@vertex
fn vs_main(
    @builtin(vertex_index) vid: u32,
    @builtin(instance_index) iid: u32,
) -> VsOut {
    let p = positions[iid];
    let speed = p.w;

    // Quad corner offsets (-1..+1) for the 6 vertices of two triangles.
    var corners = array<vec2<f32>, 6>(
        vec2<f32>(-1.0, -1.0),
        vec2<f32>( 1.0, -1.0),
        vec2<f32>(-1.0,  1.0),
        vec2<f32>(-1.0,  1.0),
        vec2<f32>( 1.0, -1.0),
        vec2<f32>( 1.0,  1.0),
    );
    let local = corners[vid];

    // Project particle center to clip space.
    let centerClip = ru.viewProj * vec4<f32>(p.xyz, 1.0);
    let viewDepth = max(centerClip.w, 0.0001);

    // Offset in NDC by point size in pixels.
    let halfPx = 0.5 * ru.pointSizePx;
    let ndcOffset = vec2<f32>(
        (local.x * halfPx * 2.0) / ru.viewportSize.x,
        (local.y * halfPx * 2.0) / ru.viewportSize.y,
    );

    // Apply offset post-projection (in clip space, the offset must be scaled by w).
    var clip = centerClip;
    clip.x = clip.x + ndcOffset.x * viewDepth;
    clip.y = clip.y + ndcOffset.y * viewDepth;

    var out: VsOut;
    out.pos = clip;
    out.localUv = local;
    out.speed = speed;
    out.viewDepth = viewDepth;
    out.colormapIndex = u32(ru.colormapIndex + 0.5);
    return out;
}
