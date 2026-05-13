// fullscreen.vert.glsl — Fullscreen triangle covering [-1,1]² via 3-vertex draw.
// Same approach Phase 8 ES uses for its raymarch fullscreen pass.
#version 460

layout(location = 0) out vec2 v_uv;

void main() {
    vec2 pos = vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2));
    v_uv = pos;
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}
