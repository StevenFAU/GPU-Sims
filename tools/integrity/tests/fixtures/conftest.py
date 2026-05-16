"""Pytest collection guard for fixture trees.

Some fixture trees (e.g. good_toolkit_self / bad_toolkit_self under v1.2
A.2) mirror the production layout including `tests/test_*.py` files.
Those files are scan-input for the toolkit's own self-application
check and are not pytest test cases; preventing pytest from collecting
them here keeps the suite clean.
"""

from __future__ import annotations

collect_ignore_glob = [
    "good_toolkit_self/**/*.py",
    "bad_toolkit_self/**/*.py",
]
