// Reaction-Diffusion 2D — BufA pass (state update).
//
// Stack A artifact for GPU-Sims. Steven-original implementation; references
// consulted (Pearson 1993 paper, Munafo's xmorphia catalog, public-domain
// Gray-Scott formulations) but no code lifted. License: MIT (matches the
// rest of the repo).
//
// Self-references via iChannel0 = BufferA. Reads previous state, writes new.
// Set iChannel0 filter = Nearest, wrap = Clamp (we wrap manually below).
//
// One Forward Euler substep per frame. Pattern formation takes ~30s at 60fps
// on the λ preset. The Stack B port runs multiple substeps per displayed
// frame; Shadertoy's single-pass-per-frame model means BufA is slower.

// Pearson 1993 named regions. Edit (F, k) below to switch presets manually:
//   λ — Irregular spots:        F = 0.026, k = 0.061
//   σ — Stripes:                F = 0.037, k = 0.060
//   α — Chaotic:                F = 0.014, k = 0.047
//   β — Uniform-ish:            F = 0.026, k = 0.055
//   ξ — Moving spots:           F = 0.018, k = 0.051
//   τ — U-skate (replicating):  F = 0.020, k = 0.052

const float F  = 0.026;
const float k  = 0.061;
const float Du = 0.16;
const float Dv = 0.08;
const float dt = 1.0;

// Initial-condition seed block size in cells. The first frame gets a small
// patch of (u=0.5, v=0.25) at the center; everywhere else is (u=1, v=0).
const float SEED_BLOCK_HALF = 8.0;
const float NOISE_AMP       = 0.05;

// Manual periodic wrap helper: takes integer pixel offset, returns wrapped
// texel coordinate in [0, iResolution).
ivec2 wrap(ivec2 p, ivec2 size) {
    return ivec2(mod(vec2(p) + vec2(size), vec2(size)));
}

// Deterministic pseudo-random in [0, 1) keyed on (x, y, salt).
float hash3(int x, int y, int salt) {
    uint h = uint(x) * 374761393u
           + uint(y) * 668265263u
           + uint(salt) * 2147483647u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);
    return float(h & 0x00FFFFFFu) / 16777216.0;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    ivec2 p    = ivec2(fragCoord);
    ivec2 size = ivec2(iResolution.xy);

    // Frame 0: write the initial conditions instead of running the kernel.
    if (iFrame == 0) {
        float u = 1.0 + (hash3(p.x, p.y, 0) - 0.5) * NOISE_AMP;
        float v = 0.0 + (hash3(p.x, p.y, 1) - 0.5) * NOISE_AMP;
        vec2 c = vec2(p) - 0.5 * vec2(size);
        if (abs(c.x) < SEED_BLOCK_HALF && abs(c.y) < SEED_BLOCK_HALF) {
            u = 0.5  + (hash3(p.x, p.y, 2) - 0.5) * NOISE_AMP;
            v = 0.25 + (hash3(p.x, p.y, 3) - 0.5) * NOISE_AMP;
        }
        fragColor = vec4(u, v, 0.0, 1.0);
        return;
    }

    // Read center + 4 neighbors (periodic wrap).
    vec2 cc = texelFetch(iChannel0, p, 0).xy;
    vec2 n  = texelFetch(iChannel0, wrap(p + ivec2( 0, -1), size), 0).xy;
    vec2 s  = texelFetch(iChannel0, wrap(p + ivec2( 0,  1), size), 0).xy;
    vec2 w  = texelFetch(iChannel0, wrap(p + ivec2(-1,  0), size), 0).xy;
    vec2 e  = texelFetch(iChannel0, wrap(p + ivec2( 1,  0), size), 0).xy;

    vec2 lap = (n + s + w + e) - 4.0 * cc;

    float u = cc.x;
    float v = cc.y;

    float reaction = u * v * v;
    float du = Du * lap.x - reaction + F * (1.0 - u);
    float dv = Dv * lap.y + reaction - (F + k) * v;

    float nu = clamp(u + dt * du, 0.0, 1.0);
    float nv = clamp(v + dt * dv, 0.0, 1.0);

    // Mouse-paint: while LMB held, splat v material around iMouse.xy.
    // iMouse.zw > 0 indicates the button is currently held.
    if (iMouse.z > 0.0) {
        vec2 cell = vec2(p);
        float d = distance(cell, iMouse.xy);
        float radius = 12.0;
        float falloff = max(0.0, 1.0 - d / radius);
        float amount = falloff * 0.5;
        nu = clamp(nu - 0.5 * amount, 0.0, 1.0);
        nv = clamp(nv + 0.5 * amount, 0.0, 1.0);
    }

    fragColor = vec4(nu, nv, 0.0, 1.0);
}
