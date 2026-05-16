"""Fixture module: consumes consumed_helper (only).

Note: this module's `main` would itself be unused, but is named `main`
which the entrypoint-convention rule exempts when the file is under a
scripts/ subdir or named __main__.py. Here it lives inside the integrity
package, so to keep the fixture purely focused on orphan_helper and
OrphanClass we wrap the call site in a module-level statement instead
of a `main` function.
"""

from integrity.module_a import consumed_helper


_RESULT = consumed_helper(0)
