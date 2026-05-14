"""Widget module."""

from dataclasses import dataclass


@dataclass
class Widget:
    name: str
    count: int

    def increment(self) -> None:
        self.count += 1


def make_widget(name: str) -> Widget:
    return Widget(name=name, count=0)
