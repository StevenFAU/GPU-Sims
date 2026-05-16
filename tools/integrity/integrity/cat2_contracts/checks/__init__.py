"""Cat 2 check modules. Discovered by integrity.runner.discover_checks."""

from integrity.cat2_contracts.checks import (
    public_symbol_used,
    public_symbol_used_b,
    public_symbol_used_c,
    public_symbol_used_toolkit,
    stub_label_stale,
)

REGISTERED_CHECKS = [
    (public_symbol_used.CHECK_ID, public_symbol_used),
    (public_symbol_used_c.CHECK_ID, public_symbol_used_c),
    (public_symbol_used_b.CHECK_ID, public_symbol_used_b),
    (stub_label_stale.CHECK_ID, stub_label_stale),
    (public_symbol_used_toolkit.CHECK_ID, public_symbol_used_toolkit),
]
