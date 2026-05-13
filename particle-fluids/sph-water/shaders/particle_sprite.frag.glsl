// particle_sprite.frag.glsl — Sphere-imposter depth writer for the depth pass.
// Writes gl_FragDepth; no color output.
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

void main() {
    vec2  uv = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(uv, uv);
    if (r2 > 1.0) discard;

    // Right-handed view space: front of camera is z < 0.
    // Front surface of sphere is at center_z + sqrt(1-r²) * radius (less negative).
    float z_offset           = sqrt(1.0 - r2) * v_view_radius;
    vec3  surface_view_pos   = v_view_pos + vec3(uv * v_view_radius, z_offset);

    vec4  clip = proj * vec4(surface_view_pos, 1.0);
    gl_FragDepth = clip.z / clip.w;
}
