#pragma once

namespace widget {

struct Widget {
    int name_len;
    int count;
};

Widget make_widget(int initial_len);

}  // namespace widget
