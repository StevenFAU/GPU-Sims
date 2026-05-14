#pragma once

#include <string>

namespace widget {

struct Widget {
    std::string name;
    int count;
};

Widget make_widget(const std::string& name);

}  // namespace widget
