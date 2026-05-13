// particle_sprite.frag.glsl — Sphere-imposter depth writer.
// Writes NDC Z to .r of the R32_SFLOAT color attachment.
// The depth-pass attachment is COLOR, not DEPTH — there is
// no depth buffer attached during this pass, so writing to
// gl_FragDepth would be a no-op for the consumed texture.
#version 460

layout(set=0, binding=0, std430) restrict readonly buffer Particles {
    vec4 p[];
};
layout(set=0, binding=1, std140) uniform RenderView {
    mat4  viewProj;
    mat4  view;
    mat4  proj;
    mat4  invViewProj;
    vec4  cameraPos_pad;
    vec4  viewport_pad;
    float particleRadius;
    float pointScale;
    float thicknessPerParticle;
    float _pad0;
};

layout(location = 0) in vec3  v_view_pos;
layout(location = 1) in float v_view_radius;

layout(location = 0) out vec4 o_color;

void main() {
    vec2  uv = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(uv, uv);
    if (r2 > 1.0) discard;

    float z_offset           = sqrt(1.0 - r2) * v_view_radius;
    vec3  surface_view_pos   = v_view_pos + vec3(uv * v_view_radius, z_offset);
    vec4  clip = proj * vec4(surface_view_pos, 1.0);
    float ndc_z = clip.z / clip.w;     // Vulkan NDC Z in [0, 1]

    o_color = vec4(ndc_z, 0.0, 0.0, 1.0);
}
