#version 450

// Volume raymarching of the v field, front-to-back compositing.
// Vertex stage is the shared fullscreen.vert.glsl (single triangle, 3 vertices).
//
// Bindings:
//   set 0 binding 0: v_field (sampler3D, LINEAR + REPEAT)
//   set 0 binding 1: lut_tex (sampler2D, 256x4 RGBA8 LUT — magma/inferno/viridis/HSV)
//   set 0 binding 2: RaymarchUniforms

layout(set = 0, binding = 0) uniform sampler3D v_field;
layout(set = 0, binding = 1) uniform sampler2D lut_tex;

layout(set = 0, binding = 2) uniform RaymarchUniforms {
    mat4  invViewProj;        // for ray reconstruction from clip space
    vec4  cameraPos;           // .xyz; .w unused
    vec4  volumeMin;           // AABB min in world space (.xyz; .w unused)
    vec4  volumeMax;           // AABB max in world space
    int   stepCount;
    float densityThreshold;
    float densityIntensity;
    float colormapIndex;       // 0..3
    float exposure;
    float bloomIntensityUnused; // composited in tonemap pass, kept for layout sym
    float _pad0;
    float _pad1;
} rm;

layout(location = 0) in vec2 inUv;     // 0..1 across screen
layout(location = 0) out vec4 outColor; // HDR -> consumed by tonemap pass

// AABB-ray intersection. Returns vec2(tNear, tFar); if tFar < tNear, miss.
vec2 ray_aabb(vec3 ro, vec3 rd, vec3 boxMin, vec3 boxMax) {
    vec3 invD = 1.0 / rd;
    vec3 t0 = (boxMin - ro) * invD;
    vec3 t1 = (boxMax - ro) * invD;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float tNear = max(max(tmin.x, tmin.y), tmin.z);
    float tFar  = min(min(tmax.x, tmax.y), tmax.z);
    return vec2(tNear, tFar);
}

void main() {
    // Reconstruct world-space ray from screen UV.
    vec2 ndc = inUv * 2.0 - 1.0;
    vec4 nearH = rm.invViewProj * vec4(ndc, 0.0, 1.0);
    vec4 farH  = rm.invViewProj * vec4(ndc, 1.0, 1.0);
    vec3 nearW = nearH.xyz / nearH.w;
    vec3 farW  = farH.xyz  / farH.w;

    vec3 ro = rm.cameraPos.xyz;
    vec3 rd = normalize(farW - nearW);

    vec2 tHit = ray_aabb(ro, rd, rm.volumeMin.xyz, rm.volumeMax.xyz);
    float tNear = max(tHit.x, 0.0);
    float tFar  = tHit.y;

    if (tFar <= tNear) {
        outColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }

    // Map the raymarcher's marched position from world AABB to texture uvw [0, 1].
    vec3 boxSize = rm.volumeMax.xyz - rm.volumeMin.xyz;
    vec3 invBox  = 1.0 / boxSize;

    int N = max(rm.stepCount, 4);
    float dt = (tFar - tNear) / float(N);

    vec3 accum = vec3(0.0);
    float alpha = 0.0;

    // Colormap LUT row (one of 4 rows). v coordinate of (idx + 0.5) / 4.
    float lutRow = (rm.colormapIndex + 0.5) / 4.0;

    for (int s = 0; s < N; ++s) {
        float t = tNear + (float(s) + 0.5) * dt;
        vec3 posW = ro + t * rd;
        vec3 uvw  = (posW - rm.volumeMin.xyz) * invBox;

        // Sample v with LINEAR + REPEAT.
        float vSample = texture(v_field, uvw).r;

        // Density transfer: smoothstep around threshold, scaled by intensity.
        float density = smoothstep(rm.densityThreshold,
                                   rm.densityThreshold + 0.1,
                                   vSample) * rm.densityIntensity;

        if (density > 0.0) {
            // Sample colormap by v itself (not density) so feature-color varies with concentration.
            float lutU = clamp(vSample, 0.0, 1.0);
            vec3 sampleColor = texture(lut_tex, vec2(lutU, lutRow)).rgb;

            // Front-to-back compositing.
            float oneMinusA = 1.0 - alpha;
            accum += oneMinusA * sampleColor * density;
            alpha += oneMinusA * density;

            if (alpha > 0.99) break;
        }
    }

    // HDR output. Tonemap pass applies exposure + Reinhard; we leave raw HDR here.
    // (Single-pass design: this fragment shader writes directly to swapchain
    // and does the tonemap at the end. See § 7.6 for the full design.)
    accum *= rm.exposure;
    vec3 mapped = accum / (vec3(1.0) + accum);

    outColor = vec4(mapped, alpha);
}
