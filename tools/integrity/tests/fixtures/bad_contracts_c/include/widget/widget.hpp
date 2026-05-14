#pragma once

namespace widget {

// Mirrors the ParticleFrame::radii defect: `radii_ptr` is declared but never read.
struct Frame {
    float* positions;
    float* radii_ptr;
    int count;
};

Frame make_frame(int n);

// Mirrors the vdb::writeVec3Grid defect: declared and implemented, never called.
void unused_function(int x);

}  // namespace widget
