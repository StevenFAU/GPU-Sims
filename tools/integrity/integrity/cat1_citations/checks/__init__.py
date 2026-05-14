"""Cat 1 check modules. Discovered by integrity.runner.discover_checks."""

from integrity.cat1_citations.checks import (
    annotation,
    intra_repo,
    unregistered_upstream,
    upstream,
    upstream_anchor,
)

REGISTERED_CHECKS = [
    (intra_repo.CHECK_ID, intra_repo),
    (annotation.CHECK_ID, annotation),
    (upstream.CHECK_ID, upstream),
    (upstream_anchor.CHECK_ID, upstream_anchor),
    (unregistered_upstream.CHECK_ID, unregistered_upstream),
]
