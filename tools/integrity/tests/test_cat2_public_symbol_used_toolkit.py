"""Tests for cat2.public-symbol-used-toolkit (v1.2 A.2)."""

from __future__ import annotations

import textwrap
from pathlib import Path

import pytest

from integrity.cat2_contracts.checks.public_symbol_used_toolkit import (
    CHECK_ID,
    MODE,
    _build_consumption_index,
    _extract_public_symbols,
    _extract_registered_check_modules,
    run,
)
from integrity.common.results import FailureMode


# ---------------------------------------------------------------------------
# Smoke
# ---------------------------------------------------------------------------


def test_check_id_and_mode() -> None:
    assert CHECK_ID == "cat2.public-symbol-used-toolkit"
    assert MODE == FailureMode.HARD_FAIL


# ---------------------------------------------------------------------------
# Extraction: what counts as a public symbol target
# ---------------------------------------------------------------------------


def _write_module(root: Path, rel: str, body: str) -> Path:
    p = root / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(textwrap.dedent(body).lstrip("\n"), encoding="utf-8")
    return p


def test_extract_public_symbols_includes_top_level_def(tmp_path: Path) -> None:
    _write_module(tmp_path, "integrity/m.py",
                  "def public_fn():\n    return 1\n")
    syms = _extract_public_symbols(tmp_path)
    names = {s.name for s in syms}
    assert "public_fn" in names


def test_extract_public_symbols_includes_top_level_class(tmp_path: Path) -> None:
    _write_module(tmp_path, "integrity/m.py",
                  "class PublicCls:\n    pass\n")
    syms = _extract_public_symbols(tmp_path)
    names = {s.name for s in syms}
    assert "PublicCls" in names


def test_extract_public_symbols_excludes_module_constants(tmp_path: Path) -> None:
    _write_module(tmp_path, "integrity/m.py",
                  "ANCHOR_FILE = 'x'\nEXPECTED_JSON = {}\n")
    syms = _extract_public_symbols(tmp_path)
    names = {s.name for s in syms}
    assert "ANCHOR_FILE" not in names
    assert "EXPECTED_JSON" not in names


def test_extract_public_symbols_excludes_underscore_prefixed(tmp_path: Path) -> None:
    _write_module(tmp_path, "integrity/m.py",
                  "def _private():\n    return 1\n"
                  "class _PrivateCls:\n    pass\n")
    syms = _extract_public_symbols(tmp_path)
    names = {s.name for s in syms}
    assert "_private" not in names
    assert "_PrivateCls" not in names


def test_extract_public_symbols_excludes_visit_methods(tmp_path: Path) -> None:
    """visit_* methods inside ast.NodeVisitor subclasses are framework-
    dispatched; even though A.2 scans only top-level def/class (so
    visit_* methods aren't surfaced at top level anyway), confirm the
    class itself is surfaced and methods are not promoted."""
    _write_module(tmp_path, "integrity/m.py",
                  "import ast\n"
                  "class MyVisitor(ast.NodeVisitor):\n"
                  "    def visit_Name(self, node): pass\n"
                  "    def visit_Call(self, node): pass\n")
    syms = _extract_public_symbols(tmp_path)
    names = {s.name for s in syms}
    assert "MyVisitor" in names
    assert "visit_Name" not in names
    assert "visit_Call" not in names


def test_extract_public_symbols_excludes_test_functions(tmp_path: Path) -> None:
    _write_module(tmp_path, "integrity/m.py",
                  "def test_thing():\n    pass\n"
                  "def real_thing():\n    pass\n")
    syms = _extract_public_symbols(tmp_path)
    names = {s.name for s in syms}
    assert "test_thing" not in names
    assert "real_thing" in names


def test_extract_public_symbols_excludes_tests_dir(tmp_path: Path) -> None:
    """Decision 3: files under tests/ are scan-input only, not scan-target."""
    _write_module(tmp_path, "tests/test_foo.py",
                  "def helper_in_test_dir():\n    pass\n")
    syms = _extract_public_symbols(tmp_path)
    names = {s.name for s in syms}
    assert "helper_in_test_dir" not in names


# ---------------------------------------------------------------------------
# Consumption: what counts as a consumer
# ---------------------------------------------------------------------------


def test_consumption_index_records_imports(tmp_path: Path) -> None:
    _write_module(tmp_path, "integrity/m.py", "def foo(): return 1\n")
    _write_module(tmp_path, "integrity/other.py",
                  "from integrity.m import foo\n")
    syms = _extract_public_symbols(tmp_path)
    idx = _build_consumption_index(tmp_path, syms)
    assert idx.get("foo")


def test_consumption_index_records_name_references(tmp_path: Path) -> None:
    _write_module(tmp_path, "integrity/m.py", "def foo(): return 1\n")
    _write_module(tmp_path, "integrity/other.py",
                  "import integrity.m as m\n"
                  "def consumer():\n    return foo()\n")
    syms = _extract_public_symbols(tmp_path)
    idx = _build_consumption_index(tmp_path, syms)
    assert idx.get("foo")


def test_consumption_index_records_attribute_access(tmp_path: Path) -> None:
    _write_module(tmp_path, "integrity/m.py",
                  "class Widget:\n    pass\n")
    _write_module(tmp_path, "integrity/other.py",
                  "import integrity.m\n"
                  "def use():\n    return integrity.m.Widget\n")
    syms = _extract_public_symbols(tmp_path)
    idx = _build_consumption_index(tmp_path, syms)
    assert idx.get("Widget")


# ---------------------------------------------------------------------------
# Reflection-aware consumption
# ---------------------------------------------------------------------------


def test_extract_registered_check_modules(tmp_path: Path) -> None:
    _write_module(tmp_path, "integrity/cat2_contracts/checks/__init__.py",
                  "from integrity.cat2_contracts.checks import (\n"
                  "    public_symbol_used_toolkit,\n"
                  "    public_symbol_used,\n"
                  ")\n\n"
                  "REGISTERED_CHECKS = [\n"
                  "    (public_symbol_used_toolkit.CHECK_ID, public_symbol_used_toolkit),\n"
                  "    (public_symbol_used.CHECK_ID, public_symbol_used),\n"
                  "]\n")
    mods = _extract_registered_check_modules(tmp_path)
    assert "public_symbol_used_toolkit" in mods
    assert "public_symbol_used" in mods


def test_registered_checks_treated_as_consumers(tmp_path: Path) -> None:
    """A module's `run`, `CHECK_ID`, `MODE` are not flagged when the
    module appears in a REGISTERED_CHECKS tuple, even if no other file
    imports those names directly."""
    _write_module(tmp_path, "integrity/cat2_contracts/checks/foo.py",
                  "from integrity.common.results import FailureMode\n"
                  "CHECK_ID = 'cat2.foo'\n"
                  "MODE = FailureMode.HARD_FAIL\n"
                  "def run(root): return []\n")
    _write_module(tmp_path, "integrity/cat2_contracts/checks/__init__.py",
                  "from integrity.cat2_contracts.checks import foo\n"
                  "REGISTERED_CHECKS = [(foo.CHECK_ID, foo)]\n")
    findings = run(tmp_path)
    flagged_names = {f.message.split("'")[1] for f in findings if "'" in f.message}
    assert "run" not in flagged_names
    assert "CHECK_ID" not in flagged_names
    assert "MODE" not in flagged_names


def test_main_in_entrypoints_treated_as_consumed(tmp_path: Path) -> None:
    """`main` in scripts/ or __main__.py is never flagged as unused."""
    _write_module(tmp_path, "scripts/myscript.py",
                  "def main():\n    return 0\n")
    _write_module(tmp_path, "integrity/__main__.py",
                  "def main():\n    return 1\n")
    findings = run(tmp_path)
    flagged = {f.message.split("'")[1] for f in findings if "'" in f.message}
    assert "main" not in flagged


# ---------------------------------------------------------------------------
# Fixture-driven coverage
# ---------------------------------------------------------------------------


def test_good_toolkit_self_yields_no_findings(fixtures_dir: Path) -> None:
    findings = run(fixtures_dir / "good_toolkit_self")
    assert findings == [], (
        f"unexpected findings: "
        f"{[(f.file, f.line, f.message) for f in findings]}"
    )


def test_bad_toolkit_self_emits_findings_for_unused(fixtures_dir: Path) -> None:
    findings = run(fixtures_dir / "bad_toolkit_self")
    names = {f.message.split("'")[1] for f in findings}
    assert "orphan_helper" in names
    assert "OrphanClass" in names


def test_bad_toolkit_self_does_not_emit_for_consumed(fixtures_dir: Path) -> None:
    findings = run(fixtures_dir / "bad_toolkit_self")
    names = {f.message.split("'")[1] for f in findings}
    assert "consumed_helper" not in names
    assert "PRIVATE_CONSTANT" not in names
    assert "_underscore_helper" not in names


# ---------------------------------------------------------------------------
# Decision-8 sanity check (slow / opt-in)
# ---------------------------------------------------------------------------


@pytest.mark.slow
def test_real_repo_finding_count_in_expected_range(repo_root: Path) -> None:
    """Per Decision 8: warn-only run on real repo yields 3 <= N <= 30."""
    findings = run(repo_root)
    count = len(findings)
    assert 3 <= count <= 30, (
        f"finding count {count} outside expected [3, 30]; "
        f"extraction strategy may be miscalibrated"
    )
