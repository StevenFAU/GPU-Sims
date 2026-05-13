// thickness.frag.glsl — Additive Gaussian thickness contribution per fragment.
// Depth test is disabled at pipeline level. Blends additively into r16f attachment.
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

layout(location = 0) in  float v_view_z;
layout(location = 0) out vec4  o_color;

void main() {
    vec2  uv = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(uv, uv);
    if (r2 > 1.0) discard;

    float profile   = exp(-r2 * 2.0);
    float thickness = thicknessPerParticle * profile * (1.0 / max(v_view_z, 0.1));
    o_color = vec4(thickness, 0.0, 0.0, 1.0);
}
