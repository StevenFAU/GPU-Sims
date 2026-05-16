"""Fixture test: imports consumed_helper only.

Notably does NOT import orphan_helper or OrphanClass; those should be
flagged by the check.
"""

from integrity.module_a import consumed_helper


def test_consumed_helper() -> None:
    assert consumed_helper(0) == 1
