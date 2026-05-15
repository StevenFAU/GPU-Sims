#version 460

// Streamline vertex shader. v1: one draw call per streamline (LINE_STRIP,
// vertexCount = history). The vertex index walks the ring buffer in age
// order so the strip is drawn from oldest to newest. The host passes the
// streamline index as a push constant (sid).
//
// Class C divergence from spec § 4.B.12: original spec used
// vkCmdDraw(streamlineCount * (history+1)) with primitive_restart_enable
// + NaN-vertex sentinels. Primitive restart only fires on indexed draws,
// so the spec form would actually emit degenerate triangles between
// strips on most drivers. v1 uses one vkCmdDraw per streamline; cost
// is N draw calls per frame (~10k at default), tractable on RDNA2.
// Banked v1.1: switch to indexed draw with explicit restart sentinel.

layout(set = 0, binding = 0, std430) readonly buffer PositionHistory {
    vec4 positions[];   // (xyz, age); length = streamline_count * history
};

layout(set = 0, binding = 1) uniform StreamlineRenderUniforms {
    mat4  viewProj;
    vec4  lineColor;
    uint  history;
    uint  head_index;       // ring head (newest position lives at slot head-1 mod H)
    float ageFalloff;
    float _pad0;
} U;

layout(push_constant) uniform PushConsts {
    uint sid;               // streamline index for this draw
} pc;

layout(location = 0) out vec4 v_color;

void main() {
    // Walk ring buffer from oldest to newest. The newest position lives at
    // slot (head_index - 1) mod history; we draw from slot head_index
    // (oldest) up through slot head_index + history - 1.
    uint hi      = (U.head_index + uint(gl_VertexIndex)) % U.history;
    vec4 p       = positions[pc.sid * U.history + hi];
    gl_Position  = U.viewProj * vec4(p.xyz, 1.0);
    float age01  = float(gl_VertexIndex) / float(U.history - 1u);  // 0..1, oldest -> newest
    float alpha  = mix(0.1, 1.0, age01);   // fade old end
    v_color      = vec4(U.lineColor.rgb, U.lineColor.a * alpha);
}
