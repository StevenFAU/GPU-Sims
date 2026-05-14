#include "widget/widget.hpp"

int use_widget() {
    auto w = widget::make_widget(5);
    return w.name_len + w.count;
}
