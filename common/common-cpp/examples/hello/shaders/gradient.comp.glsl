#version 460

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(set = 0, binding = 0, rgba8) uniform writeonly image2D outImage;

layout(push_constant) uniform Push {
    vec2  resolution;
    float time;
    float _pad;
} pc;

void main() {
    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    if (p.x >= int(pc.resolution.x) || p.y >= int(pc.resolution.y)) return;

    vec2  uv = (vec2(p) + 0.5) / pc.resolution;
    float t  = pc.time;

    // Smooth animated gradient with a soft swirl, kept tame so it's pleasant
    // to look at while you tinker with the rest of the wiring.
    float r = 0.5 + 0.5 * sin(uv.x * 6.28318 + t * 0.6);
    float g = 0.5 + 0.5 * sin(uv.y * 6.28318 + t * 0.4 + 2.094);
    float b = 0.5 + 0.5 * sin((uv.x + uv.y) * 6.28318 + t * 0.5 + 4.188);

    imageStore(outImage, p, vec4(r, g, b, 1.0));
}
