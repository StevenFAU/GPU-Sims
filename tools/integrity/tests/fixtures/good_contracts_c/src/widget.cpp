#include "widget/widget.hpp"

namespace widget {

Widget make_widget(int initial_len) {
    return Widget{initial_len, 0};
}

}  // namespace widget
