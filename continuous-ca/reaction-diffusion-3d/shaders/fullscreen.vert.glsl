#version 450

// Single-triangle fullscreen vertex shader (no vertex buffer).
// Identical to common-cpp/examples/hello/shaders/fullscreen.vert.glsl;
// duplicated here to keep the sim self-contained.

layout(location = 0) out vec2 outUv;

void main() {
    // Three vertices producing a triangle that covers the [-1, 1]² screen.
    vec2 positions[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    vec2 uvs[3] = vec2[3](
        vec2(0.0, 0.0),
        vec2(2.0, 0.0),
        vec2(0.0, 2.0)
    );

    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    outUv       = uvs[gl_VertexIndex];
}
