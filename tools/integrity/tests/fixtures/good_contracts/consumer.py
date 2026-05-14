"""Synthetic consumer. Exercises every public symbol in the fixture's gpusims_common."""

from gpusims_common import Widget, make_widget


def use_them() -> int:
    w: Widget = make_widget("hello")
    w.increment()
    print(w.name)
    return w.count
