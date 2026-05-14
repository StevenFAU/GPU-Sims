#include "widget/widget.hpp"

int use_frame() {
    auto f = widget::make_frame(10);
    return static_cast<int>(f.positions.size()) + f.count;
}
