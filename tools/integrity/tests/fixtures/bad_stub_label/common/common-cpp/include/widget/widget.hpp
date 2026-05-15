#pragma once

namespace widget {

// Widget operations.
//
// In Phase 1, this is a stub: if WIDGET_REAL is not defined at compile
// time, all functions log a warning and return false.

struct Frame {
    int count = 0;
};

bool write_frame(const Frame& f);

}  // namespace widget
