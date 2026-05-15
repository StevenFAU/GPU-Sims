"""Cat 3 check modules. Discovered by integrity.runner.discover_checks."""

from integrity.cat3_numerical.checks import (
    cubic_kernel,
    d3q19_velocity_set,
    d3q19_weights,
    d3q19_equilibrium,
)

REGISTERED_CHECKS = [
    (cubic_kernel.CHECK_ID, cubic_kernel),
    (d3q19_velocity_set.CHECK_ID, d3q19_velocity_set),
    (d3q19_weights.CHECK_ID, d3q19_weights),
    (d3q19_equilibrium.CHECK_ID, d3q19_equilibrium),
]
