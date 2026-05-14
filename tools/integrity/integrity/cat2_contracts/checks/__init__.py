"""Cat 2 check modules. Discovered by integrity.runner.discover_checks."""

from integrity.cat2_contracts.checks import public_symbol_used, public_symbol_used_c

REGISTERED_CHECKS = [
    (public_symbol_used.CHECK_ID, public_symbol_used),
    (public_symbol_used_c.CHECK_ID, public_symbol_used_c),
]
