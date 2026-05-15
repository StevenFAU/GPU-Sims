"""Cat 3 check modules. Discovered by integrity.runner.discover_checks."""

from integrity.cat3_numerical.checks import cubic_kernel

REGISTERED_CHECKS = [
    (cubic_kernel.CHECK_ID, cubic_kernel),
]
