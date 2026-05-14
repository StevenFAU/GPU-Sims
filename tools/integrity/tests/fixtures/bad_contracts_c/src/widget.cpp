#include "widget/widget.hpp"

namespace widget {

Frame make_frame(int n) {
    Frame f;
    f.positions.resize(static_cast<size_t>(n) * 3);
    // Note: f.radii is intentionally never touched here — this fixture
    // mirrors the silent-data-loss defect where the field is declared
    // but no consumer ever reads or writes it.
    f.count = n;
    return f;
}

void unused_function(int x) {
    static_cast<void>(x);
}

}  // namespace widget
