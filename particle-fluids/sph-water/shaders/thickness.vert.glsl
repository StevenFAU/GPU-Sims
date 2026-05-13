// thickness.vert.glsl — Vertex stage for the additive thickness pass.
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

layout(location = 0) out float v_view_z;

void main() {
    uint vid       = uint(gl_VertexIndex);
    vec3 world_pos = p[vid * 8u + 0u].xyz;
    vec4 view_pos4 = view * vec4(world_pos, 1.0);

    v_view_z = -view_pos4.z;
    gl_Position  = proj * view_pos4;
    gl_PointSize = max(2.0, 2.0 * particleRadius * pointScale / max(v_view_z, 0.01));
}
