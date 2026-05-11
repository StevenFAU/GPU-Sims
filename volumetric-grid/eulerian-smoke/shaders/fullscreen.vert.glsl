#version 450
#extension GL_GOOGLE_include_directive : require

// Standard fullscreen-triangle vertex shader. Draws a single oversize triangle
// whose interior covers the entire viewport. UV is in [0,1]² across the visible
// region. Same convention as common-cpp/examples/hello and reaction-diffusion-3d.

layout(location = 0) out vec2 v_uv;

void main() {
    // Triangle vertices in clip space:
    //   gl_VertexIndex = 0  ->  (-1, -1)   uv = (0, 0)
    //   gl_VertexIndex = 1  ->  ( 3, -1)   uv = (2, 0)
    //   gl_VertexIndex = 2  ->  (-1,  3)   uv = (0, 2)
    // The triangle is clipped to the [-1, 1] viewport so the visible region
    // sees uv ∈ [0, 1] × [0, 1].
    vec2 pos = vec2(
        (gl_VertexIndex & 1) == 1 ? 3.0 : -1.0,
        (gl_VertexIndex & 2) == 2 ? 3.0 : -1.0
    );
    v_uv = (pos + 1.0) * 0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
}
