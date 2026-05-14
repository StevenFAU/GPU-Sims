#include "widget/widget.hpp"

int use_frame() {
    auto f = widget::make_frame(10);
    int n = 0;
    if (f.positions != nullptr) {
        n += 1;
    }
    return n + f.count;
}
