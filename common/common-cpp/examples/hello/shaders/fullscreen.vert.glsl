#version 460

// Emits a single triangle that covers the entire viewport. No vertex buffer.
// gl_VertexIndex 0,1,2 → screen-space corners.
layout(location = 0) out vec2 v_uv;

void main() {
    vec2 verts[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    vec2 uvs[3] = vec2[](
        vec2(0.0, 0.0),
        vec2(2.0, 0.0),
        vec2(0.0, 2.0)
    );
    gl_Position = vec4(verts[gl_VertexIndex], 0.0, 1.0);
    v_uv        = uvs[gl_VertexIndex];
}
