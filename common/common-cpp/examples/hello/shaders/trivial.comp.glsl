#version 460

layout(local_size_x = 32) in;

// Phase 11 sph-water smoke-test kernel. Writes the invocation index into a
// 1-element SSBO so the binding is non-dead. Exists to validate that a
// compute pipeline with required_subgroup_size pinned can be created — the
// pipeline-creation path exercises the new pNext-chain logic in
// compute_pipeline.cpp; pipeline-creation success is the load-bearing check.
layout(std430, binding = 0) buffer Sink {
    uint values[];
} sink;

void main() {
    if (gl_GlobalInvocationID.x == 0u) {
        sink.values[0] = 1u;
    }
}
