"""Fixture module: consumes helper_a and declares helper_b, consumed by module_c."""

from integrity.module_a import helper_a


def helper_b() -> int:
    return helper_a(21)
