#include "widget/widget.hpp"

#include <iostream>

namespace widget {

namespace {

bool write_frame_real(const Frame& f) {
    std::cout << "writing " << f.count << " entries\n";
    return true;
}

bool write_frame_stub(const Frame& f) {
    (void)f;
    return false;
}

}  // namespace

bool write_frame(const Frame& f) {
#ifdef WIDGET_REAL
    return write_frame_real(f);
#else
    return write_frame_stub(f);
#endif
}

}  // namespace widget
