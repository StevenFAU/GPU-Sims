#pragma once

#include <string>
#include <vector>

namespace widget {

// Mirrors the ParticleFrame::radii defect: radii is declared but never read.
struct Frame {
    std::vector<float> positions;
    std::vector<float> radii;
    int count;
};

Frame make_frame(int n);

// Mirrors the vdb::writeVec3Grid defect: declared and implemented, never called.
void unused_function(int x);

}  // namespace widget
