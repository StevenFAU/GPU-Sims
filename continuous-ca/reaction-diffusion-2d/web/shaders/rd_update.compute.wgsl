// Reaction-Diffusion 2D update kernel.
// Forward Euler step on the Gray-Scott PDEs:
//   du/dt = Du * lap(u) - u*v*v + F*(1 - u)
//   dv/dt = Dv * lap(v) + u*v*v - (F + k)*v
//
// 5-point Laplacian (center + 4 neighbors) via NEAREST sampling on a
// REPEAT-addressed sampler — periodic boundary conditions come for free.

struct RDParams {
    gridSize:  vec2<u32>,
    feedKill:  vec2<f32>,    // F, k
    diffusion: vec2<f32>,    // Du, Dv
    dt:        f32,
    _pad:      f32,
};

@group(0) @binding(0) var<uniform> params: RDParams;
@group(0) @binding(1) var srcTex: texture_2d<f32>;
@group(0) @binding(2) var srcSampler: sampler;
@group(0) @binding(3) var dstTex: texture_storage_2d<rg32float, write>;

@compute @workgroup_size(16, 16, 1)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
    if (gid.x >= params.gridSize.x || gid.y >= params.gridSize.y) {
        return;
    }

    let invSize  = 1.0 / vec2<f32>(params.gridSize);
    let centerUV = (vec2<f32>(gid.xy) + 0.5) * invSize;

    let cc = textureSampleLevel(srcTex, srcSampler, centerUV,                                            0.0).xy;
    let n  = textureSampleLevel(srcTex, srcSampler, centerUV + vec2<f32>(0.0,        -invSize.y),        0.0).xy;
    let s  = textureSampleLevel(srcTex, srcSampler, centerUV + vec2<f32>(0.0,         invSize.y),        0.0).xy;
    let w  = textureSampleLevel(srcTex, srcSampler, centerUV + vec2<f32>(-invSize.x,  0.0      ),        0.0).xy;
    let e  = textureSampleLevel(srcTex, srcSampler, centerUV + vec2<f32>( invSize.x,  0.0      ),        0.0).xy;

    let lap = (n + s + w + e) - 4.0 * cc;

    let u  = cc.x;
    let v  = cc.y;
    let F  = params.feedKill.x;
    let k  = params.feedKill.y;
    let Du = params.diffusion.x;
    let Dv = params.diffusion.y;
    let dt = params.dt;

    let reaction = u * v * v;
    let du = Du * lap.x - reaction + F * (1.0 - u);
    let dv = Dv * lap.y + reaction - (F + k) * v;

    let nu = clamp(u + dt * du, 0.0, 1.0);
    let nv = clamp(v + dt * dv, 0.0, 1.0);

    textureStore(dstTex, vec2<i32>(gid.xy), vec4<f32>(nu, nv, 0.0, 0.0));
}
