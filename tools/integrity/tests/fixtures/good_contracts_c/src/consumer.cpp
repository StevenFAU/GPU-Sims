#include "widget/widget.hpp"

int use_widget() {
    auto w = widget::make_widget("hello");
    return static_cast<int>(w.name.size()) + w.count;
}
