#version 450
#extension GL_GOOGLE_include_directive : require

// Volume raymarch fragment shader.
// Per-ray integral: see phase8_eulerian_smoke.md § 2.12 for the math.
//
// Inputs:
//   - u_density        sampler3D r32f         smoke density field
//   - u_temperature    sampler3D r32f         temperature field
//   - u_blackbody_lut  sampler2D rgba8        256x4 LUT (4 color ramps; row by colorRamp idx)
//   - u_bluenoise      sampler2D r8           256x256 blue-noise jitter pattern
//
// Output: tonemap result in the swapchain color attachment.

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler3D u_density;
layout(set = 0, binding = 1) uniform sampler3D u_temperature;
layout(set = 0, binding = 2) uniform sampler2D u_blackbody_lut;
layout(set = 0, binding = 3) uniform sampler2D u_bluenoise;

layout(set = 0, binding = 4) uniform RaymarchUniforms {
    mat4  invViewProj;
    vec4  cameraPos;          // .xyz = pos, .w = unused
    vec4  volumeMin;          // .xyz = (0,0,0), .w = unused
    vec4  volumeMax;          // .xyz = (1,1,1), .w = unused
    vec4  lightDir;           // .xyz = normalized, .w = unused
    vec4  lightColor;         // .xyz, .w = ambient strength
    vec4  bgTopColor;         // .xyz, .w = unused
    vec4  bgBottomColor;      // .xyz, .w = unused
    int   raymarchSteps;
    int   shadowMarchSteps;
    float densityAbsorption;
    float emissionStrength;
    float scatteringStrength;
    float exposure;
    float colorRampRow;       // 0..3 selects LUT row (blackbody / sunset / cold / mono)
    float shadowMarchSoftness;
} rm;

// Slab/AABB intersection: returns (t_near, t_far) for the ray's intersection with the unit cube.
// Returns (1, -1) on miss (which the caller treats as no-hit).
vec2 intersectBox(vec3 origin, vec3 dir, vec3 boxMin, vec3 boxMax) {
    vec3 invDir = 1.0 / dir;
    vec3 t0 = (boxMin - origin) * invDir;
    vec3 t1 = (boxMax - origin) * invDir;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float t_near = max(max(tmin.x, tmin.y), tmin.z);
    float t_far  = min(min(tmax.x, tmax.y), tmax.z);
    return vec2(t_near, t_far);
}

vec3 sampleBlackbody(float t) {
    float tClamp = clamp(t * 0.5, 0.0, 1.0);   // t in [0, 2] mapped to LUT u in [0, 1]
    float rowU = (rm.colorRampRow + 0.5) / 4.0;
    return texture(u_blackbody_lut, vec2(tClamp, rowU)).rgb;
}

void main() {
    // Reconstruct ray from clip-space.
    vec2 ndc = v_uv * 2.0 - 1.0;
    // WebGPU-style Y points up (matches gl-matrix perspectiveZO). No flip here.
    vec4 near_h = rm.invViewProj * vec4(ndc, 0.0, 1.0);
    vec4 far_h  = rm.invViewProj * vec4(ndc, 1.0, 1.0);
    vec3 ray_origin = near_h.xyz / near_h.w;
    vec3 ray_dir    = normalize((far_h.xyz / far_h.w) - ray_origin);

    // Background gradient (vertical, top -> bottom).
    vec3 bg = mix(rm.bgBottomColor.xyz, rm.bgTopColor.xyz, clamp(v_uv.y, 0.0, 1.0));

    // Box intersection in normalized [0,1]^3 space.
    vec2 t_range = intersectBox(ray_origin, ray_dir, rm.volumeMin.xyz, rm.volumeMax.xyz);
    float t_near = max(t_range.x, 0.0);
    float t_far  = t_range.y;
    if (t_far <= t_near) {
        outColor = vec4(bg, 1.0);
        return;
    }

    // Blue-noise jitter to break slice-banding (one tap per pixel).
    ivec2 bn_coord = ivec2(gl_FragCoord.xy) & ivec2(255);
    float jitter = texelFetch(u_bluenoise, bn_coord, 0).r;

    float step_size = (t_far - t_near) / float(rm.raymarchSteps);
    float t = t_near + jitter * step_size;

    vec3 L = vec3(0.0);
    float T = 1.0;
    vec3 lightDir = normalize(rm.lightDir.xyz);
    float ambient = rm.lightColor.w;

    // Shadow-march step size: probe one-volume-diagonal worth of distance across N steps.
    float shadow_step = (1.732 / float(rm.shadowMarchSteps));   // sqrt(3) for unit cube diagonal

    for (int i = 0; i < rm.raymarchSteps; ++i) {
        if (T < 0.01) break;   // early-out

        vec3 pos = ray_origin + t * ray_dir;
        float density     = texture(u_density,     pos).r;
        float temperature = texture(u_temperature, pos).r;

        if (density > 0.001) {
            float absorption = density * rm.densityAbsorption;
            float sample_T   = exp(-absorption * step_size);

            // Emission contribution (temperature -> black-body color).
            vec3 L_e = rm.emissionStrength * sampleBlackbody(temperature) * density;

            // Shadow march toward the key light.
            float shadow_T = 1.0;
            float shadow_t = shadow_step;
            for (int s = 0; s < rm.shadowMarchSteps; ++s) {
                vec3 shadow_pos = pos + shadow_t * lightDir;
                if (any(lessThan(shadow_pos, rm.volumeMin.xyz)) ||
                    any(greaterThan(shadow_pos, rm.volumeMax.xyz))) break;
                float shadow_density = texture(u_density, shadow_pos).r;
                shadow_T *= exp(-shadow_density * rm.densityAbsorption * shadow_step * rm.shadowMarchSoftness);
                if (shadow_T < 0.01) break;
                shadow_t += shadow_step;
            }

            // Scattering contribution.
            vec3 L_s = rm.scatteringStrength * shadow_T * rm.lightColor.xyz * density;

            // Ambient (uniform soft-light fill).
            vec3 L_a = ambient * density * rm.lightColor.xyz;

            // Accumulate (front-to-back compositing).
            L += T * step_size * (L_e + L_s + L_a);
            T *= sample_T;
        }

        t += step_size;
    }

    // Alpha-blend with background by remaining transmittance.
    L += T * bg;

    // Inline Reinhard tonemap.
    L = L * rm.exposure;
    L = L / (L + 1.0);

    outColor = vec4(L, 1.0);
}
