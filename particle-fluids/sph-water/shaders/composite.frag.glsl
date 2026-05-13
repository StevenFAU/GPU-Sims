// composite.frag.glsl — Final screen-space-fluid composite.
//
// Reads smoothed depth + thickness. Reconstructs view-space normal via
// cross-product of world-space depth derivatives (Müller-Fetterer 2007 sec 3.1).
// Schlick's Fresnel with F0 = 0.02 for water. Beer-Lambert thickness attenuation
// against a procedural two-color sky. Refraction via Snell's law (IOR 1.33).
#version 460

layout(set=0, binding=0) uniform texture2D smoothedDepth;
layout(set=0, binding=1) uniform texture2D thicknessTex;
layout(set=0, binding=2) uniform sampler   samp;
layout(set=0, binding=3, std140) uniform U {
    mat4  invViewProj;
    mat4  invView;
    vec4  cameraPos_pad;
    vec4  waterTint_roughness;
    vec4  skyZenith_pad;
    vec4  skyHorizon_pad;
    vec4  absorption_thickness_pad;
    vec4  viewport_pad;
    float near_plane;
    float far_plane;
    float exposure;
    float fresnel_F0;
};

layout(location = 0) in  vec2 v_uv;
layout(location = 0) out vec4 o_color;

vec3 reconstructWorldPos(vec2 uv, float ndc_z) {
    vec4 ndc   = vec4(uv * 2.0 - 1.0, ndc_z, 1.0);
    vec4 world = invViewProj * ndc;
    return world.xyz / world.w;
}

vec3 sampleSky(vec3 dir) {
    float t = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
    return mix(skyHorizon_pad.xyz, skyZenith_pad.xyz, smoothstep(0.0, 0.7, t));
}

float fresnelSchlick(float cosTheta, float F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    float depth = texture(sampler2D(smoothedDepth, samp), v_uv).r;
    if (depth >= 0.99999) {
        vec3 dir = normalize(reconstructWorldPos(v_uv, 1.0) - cameraPos_pad.xyz);
        o_color = vec4(sampleSky(dir) * exposure, 1.0);
        return;
    }

    vec3 P    = reconstructWorldPos(v_uv, depth);
    vec3 dpdx = dFdx(P);
    vec3 dpdy = dFdy(P);
    vec3 N    = normalize(cross(dpdy, dpdx));

    vec3  V        = normalize(cameraPos_pad.xyz - P);
    float cosTheta = max(0.0, dot(N, V));

    float thickness   = texture(sampler2D(thicknessTex, samp), v_uv).r * absorption_thickness_pad.y;
    float absorption  = absorption_thickness_pad.x;
    vec3  transmission = exp(-absorption * thickness * (vec3(1.0) - waterTint_roughness.xyz));

    vec3 refr_dir   = refract(-V, N, 1.0 / 1.33);
    vec3 refr_color = sampleSky(refr_dir);
    vec3 refl_dir   = reflect(-V, N);
    vec3 refl_color = sampleSky(refl_dir);

    float F     = fresnelSchlick(cosTheta, fresnel_F0);
    vec3  body  = refr_color * transmission + waterTint_roughness.xyz * (vec3(1.0) - transmission);
    vec3  color = mix(body, refl_color, F);

    vec3 rough_sky = mix(refl_color, skyZenith_pad.xyz, waterTint_roughness.w);
    color = mix(color, rough_sky, waterTint_roughness.w * 0.3);

    o_color = vec4(color * exposure, 1.0);
}
