"""Cat 1 check modules. Discovered by integrity.runner.discover_checks."""

from integrity.cat1_citations.checks import annotation, intra_repo

# Registry: ordered list of (check_id, module-with-run-function) pairs.
REGISTERED_CHECKS = [
    (intra_repo.CHECK_ID, intra_repo),
    (annotation.CHECK_ID, annotation),
]
