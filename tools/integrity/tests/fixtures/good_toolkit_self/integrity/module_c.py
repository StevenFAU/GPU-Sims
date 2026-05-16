"""Fixture module: consumes helper_b (the last symbol that needs a consumer)."""

from integrity.module_b import helper_b


def main() -> int:
    return helper_b()
