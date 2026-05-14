#include "widget/widget.hpp"

namespace widget {

Frame make_frame(int n) {
    Frame f;
    f.positions = nullptr;
    // Note: f.radii_ptr is intentionally never touched here — this fixture
    // mirrors the silent-data-loss defect where the field is declared but
    // no consumer ever reads or writes it.
    f.count = n;
    return f;
}

void unused_function(int x) {
    static_cast<void>(x);
}

}  // namespace widget
