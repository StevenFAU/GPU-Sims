// Reads from the post-splat HDR accumulator at full res, writes to the
// half-res bloom buffer with a soft threshold extract.
// Workgroup_size 8x8 lines up with half-res tile boundaries.

@group(0) @binding(0) var srcSampler: sampler;
@group(0) @binding(1) var srcTexture: texture_2d<f32>;
@group(0) @binding(2) var dstTexture: texture_storage_2d<rgba16float, write>;

struct BloomUniforms {
    threshold: f32,
    softKnee: f32,
    intensity: f32,    // unused here; applied at composite
    _pad: f32,
};
@group(0) @binding(3) var<uniform> bu: BloomUniforms;

@compute @workgroup_size(8, 8, 1)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
    let dstSize = textureDimensions(dstTexture);
    if (gid.x >= dstSize.x || gid.y >= dstSize.y) {
        return;
    }

    // Half-res sample point in source uv.
    let uv = (vec2<f32>(gid.xy) + 0.5) / vec2<f32>(dstSize);
    let s = textureSampleLevel(srcTexture, srcSampler, uv, 0.0);

    // Soft-knee threshold (Karis-style).
    let brightness = max(max(s.r, s.g), s.b);
    let knee = bu.softKnee;
    let curve = vec3<f32>(bu.threshold - knee, knee * 2.0, 0.25 / max(knee, 0.0001));
    let rq = clamp(brightness - curve.x, 0.0, curve.y);
    let rqq = (rq * rq) * curve.z;
    let mult = max(rqq, brightness - bu.threshold) / max(brightness, 0.0001);
    let outRgb = s.rgb * mult;

    textureStore(dstTexture, vec2<i32>(gid.xy), vec4<f32>(outRgb, 1.0));
}
