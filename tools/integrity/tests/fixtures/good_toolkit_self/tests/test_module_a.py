"""Fixture test: imports helper_a; counts as scan-input consumption."""

from integrity.module_a import helper_a


def test_helper_a_doubles() -> None:
    assert helper_a(3) == 6
