"""Fixture module: mixes consumed and unconsumed public symbols.

Expected check behavior:
  - `consumed_helper` -> consumed by module_b, NOT flagged
  - `orphan_helper`   -> no consumer, FLAGGED
  - `OrphanClass`     -> no consumer, FLAGGED
  - `PRIVATE_CONSTANT` -> module-level constant, NOT scanned
  - `_underscore_helper` -> underscore-prefixed, NOT scanned
"""


def consumed_helper(x: int) -> int:
    return x + 1


def orphan_helper(x: int) -> int:
    return x - 1


class OrphanClass:
    pass


PRIVATE_CONSTANT = 42


def _underscore_helper() -> None:
    return None
