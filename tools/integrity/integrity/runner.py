"""Top-level runner: parse CLI, discover checks, dispatch, summarize.

Commit 1 ships a stub runner. The check-discovery and dispatch logic
is structured but returns an empty findings list, since no checks are
registered yet. Commits 2+ will register check modules.

See docs/integrity-toolkit-spec.md § 5 for the full CLI surface.
"""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from integrity.common.exclusions import CANONICAL_EXCLUSIONS  # noqa: F401  (used in later commits)
from integrity.common.repo import find_repo_root, git_head_sha
from integrity.common.results import FailureMode, Finding, RunSummary


# Exit codes per spec § 5.2
EXIT_OK = 0
EXIT_HARD_FAIL = 1
EXIT_INTERNAL_FAIL = 2
EXIT_BAD_CLI = 64


@dataclass
class CliArgs:
    cat: int | None
    check: str | None
    mode: str
    root: Path
    output: str
    no_audit_log: bool


def parse_args(argv: list[str]) -> CliArgs:
    parser = argparse.ArgumentParser(
        prog="integrity",
        description="GPU-Sims integrity toolkit — cross-stack verification",
    )
    parser.add_argument("--cat", type=int, choices=[1, 2, 3], default=None,
                        help="Run only the named category")
    parser.add_argument("--check", type=str, default=None,
                        help="Run only the named check, e.g. cat1.upstream-anchor")
    parser.add_argument("--mode", choices=["strict", "warn-only"], default="strict",
                        help="strict honors HARD_FAIL/SOFT_WARN; warn-only converts all to SOFT_WARN")
    parser.add_argument("--root", type=Path, default=None,
                        help="Override repo root (default: auto-detect via git)")
    parser.add_argument("--output", choices=["human", "json", "github"], default="human",
                        help="Output format")
    parser.add_argument("--no-audit-log", action="store_true",
                        help="Skip writing to integrity_failures_<date>.md")

    ns = parser.parse_args(argv)
    return CliArgs(
        cat=ns.cat,
        check=ns.check,
        mode=ns.mode,
        root=ns.root if ns.root else find_repo_root(),
        output=ns.output,
        no_audit_log=ns.no_audit_log,
    )


def discover_checks(args: CliArgs) -> list[Any]:
    """Discover registered check modules per --cat / --check filters.

    Commit 1: no checks registered. Returns []. Commits 2+ populate
    the check registry by importing per-category modules.
    """
    return []


def run_checks(checks: list[Any], args: CliArgs) -> list[Finding]:
    """Execute the given checks. Commit 1: no-op (no checks)."""
    return []


def emit_output(summary: RunSummary, findings: list[Finding], args: CliArgs) -> None:
    """Emit results in the chosen format."""
    if args.output == "json":
        payload = {
            "schema_version": 1,
            "commit": git_head_sha(args.root),
            "summary": {
                "pass": summary.passes,
                "soft_warn": summary.soft_warns,
                "hard_fail": summary.hard_fails,
                "suppressed": summary.suppressions,
            },
            "findings": [f.to_dict() for f in findings],
        }
        json.dump(payload, sys.stdout, indent=2)
        sys.stdout.write("\n")
    elif args.output == "github":
        for f in findings:
            kind = "error" if f.mode == FailureMode.HARD_FAIL else "warning"
            sys.stdout.write(
                f"::{kind} file={f.file},line={f.line}::{f.check_id}: {f.message}\n"
            )
        _emit_human_summary(summary)
    else:
        _emit_human_summary(summary)
        for f in findings:
            sys.stdout.write(f"  {f.mode.name}: {f.check_id} at {f.file}:{f.line}\n")
            sys.stdout.write(f"    {f.message}\n")


def _emit_human_summary(summary: RunSummary) -> None:
    sys.stdout.write(
        f"integrity: {summary.passes} pass, "
        f"{summary.soft_warns} soft-warn, "
        f"{summary.hard_fails} hard-fail, "
        f"{summary.suppressions} suppressed\n"
    )


def main(argv: list[str]) -> int:
    try:
        args = parse_args(argv)
    except SystemExit as e:
        return EXIT_BAD_CLI if e.code != 0 else EXIT_OK

    try:
        checks = discover_checks(args)
        findings = run_checks(checks, args)
        summary = RunSummary(
            passes=0,
            soft_warns=sum(1 for f in findings if f.mode == FailureMode.SOFT_WARN),
            hard_fails=sum(1 for f in findings if f.mode == FailureMode.HARD_FAIL and not f.suppressed),
            suppressions=sum(1 for f in findings if f.suppressed),
        )
        emit_output(summary, findings, args)

        if summary.hard_fails > 0 and args.mode == "strict":
            return EXIT_HARD_FAIL
        return EXIT_OK
    except Exception as e:
        sys.stderr.write(f"integrity: INTERNAL_FAIL: {type(e).__name__}: {e}\n")
        import traceback
        traceback.print_exc(file=sys.stderr)
        return EXIT_INTERNAL_FAIL
