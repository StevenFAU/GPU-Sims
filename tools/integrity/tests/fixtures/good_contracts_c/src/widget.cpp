#include "widget/widget.hpp"

namespace widget {

Widget make_widget(const std::string& name) {
    return Widget{name, 0};
}

}  // namespace widget
