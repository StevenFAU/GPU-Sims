// bilateral_smooth.comp.glsl — Separable bilateral filter for screen-space
// fluid depth smoothing (Müller-Fetterer 2007 / Eisemann-Décoret 2006).
// Dispatched twice per smoothing iteration: passDirection=0 (horizontal),
// then passDirection=1 (vertical), N times in the host loop.
#version 460

layout(local_size_x = 16, local_size_y = 16) in;

layout(set=0, binding=0) uniform texture2D inputDepth;
layout(set=0, binding=1, r32f) uniform writeonly image2D outputDepth;
layout(set=0, binding=2) uniform sampler samp;
layout(set=0, binding=3, std140) uniform U {
    vec2  viewport_size;
    float sigmaSpatial;
    float sigmaDepth;
    int   passDirection;
    int   _pad0;
    vec2  _pad1;
};

void main() {
    ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    if (pix.x >= int(viewport_size.x) || pix.y >= int(viewport_size.y)) return;

    vec2  uv_center    = (vec2(pix) + 0.5) / viewport_size;
    float center_depth = texture(sampler2D(inputDepth, samp), uv_center).r;

    if (center_depth >= 0.99999) {
        imageStore(outputDepth, pix, vec4(center_depth));
        return;
    }

    int   radius       = int(ceil(2.0 * sigmaSpatial));
    float weight_sum   = 0.0;
    float depth_sum    = 0.0;
    float inv_2sig2_s  = 1.0 / (2.0 * sigmaSpatial * sigmaSpatial);
    float inv_2sig2_d  = 1.0 / (2.0 * sigmaDepth   * sigmaDepth);
    ivec2 step_dir     = (passDirection == 0) ? ivec2(1, 0) : ivec2(0, 1);

    for (int dr = -radius; dr <= radius; ++dr) {
        ivec2 spx = pix + dr * step_dir;
        if (spx.x < 0 || spx.y < 0 ||
            spx.x >= int(viewport_size.x) || spx.y >= int(viewport_size.y))
            continue;
        vec2  uv = (vec2(spx) + 0.5) / viewport_size;
        float d  = texture(sampler2D(inputDepth, samp), uv).r;
        if (d >= 0.99999) continue;
        float ws = exp(-float(dr * dr) * inv_2sig2_s);
        float dd = d - center_depth;
        float wd = exp(-(dd * dd) * inv_2sig2_d);
        float w  = ws * wd;
        depth_sum  += d * w;
        weight_sum += w;
    }

    float result = (weight_sum > 1e-6) ? (depth_sum / weight_sum) : center_depth;
    imageStore(outputDepth, pix, vec4(result));
}
