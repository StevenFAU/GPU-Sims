"""Tests for cat1.upstream-anchor."""

from __future__ import annotations

import subprocess
from pathlib import Path

from integrity.cat1_citations.checks.upstream_anchor import run
from integrity.common.results import FailureMode


def _init_synthetic_vendor(root: Path, target_sha: str | None) -> None:
    """Create a git repo at root/references/SyntheticUpstream/ with one
    commit. If target_sha is provided, the actual head sha won't match
    (we use a fixed commit message so the SHA is determined by content
    + author + time; we don't pin it. The test asserts the diagnostic
    text instead of exact SHA equality)."""
    vendor = root / "references" / "SyntheticUpstream"
    vendor.mkdir(parents=True, exist_ok=True)
    (vendor / "foo.cpp").write_text(
        "// Synthetic vendor-tree fixture file.\nint line_two();\n"
        "int line_three();\nint line_four();\n"
    )
    subprocess.run(["git", "init", "-q"], cwd=vendor, check=True)
    subprocess.run(["git", "-C", str(vendor), "config", "user.email", "t@t"], check=True)
    subprocess.run(["git", "-C", str(vendor), "config", "user.name", "t"], check=True)
    subprocess.run(["git", "-C", str(vendor), "add", "foo.cpp"], check=True)
    subprocess.run(
        ["git", "-C", str(vendor), "commit", "-q", "-m", "init"],
        check=True,
        env={"GIT_AUTHOR_DATE": "2026-05-14T00:00:00Z",
             "GIT_COMMITTER_DATE": "2026-05-14T00:00:00Z",
             "PATH": "/usr/bin:/bin"},
    )


def test_anchor_mismatch_is_flagged(tmp_path: Path) -> None:
    """Registry SHA is all-zeros; vendor HEAD will be a real SHA; expect HARD_FAIL."""
    (tmp_path / "tools" / "integrity" / "docs").mkdir(parents=True, exist_ok=True)
    (tmp_path / "tools" / "integrity" / "docs" / "ground-truth-sources.md").write_text(
        "# fixture\n\n```toml\n"
        "[SyntheticUpstream]\n"
        'anchor_version = "1.0.0"\n'
        'anchor_sha     = "0000000000000000000000000000000000000000"\n'
        'vendor_root    = "references/SyntheticUpstream"\n'
        'anchor_doc     = "README.md"\n'
        'upstream_url   = "https://example.com"\n'
        'used_by_checks = ["cat1.upstream-anchor"]\n'
        "```\n"
    )
    _init_synthetic_vendor(tmp_path, target_sha=None)
    findings = run(tmp_path)
    assert len(findings) == 1
    assert findings[0].mode == FailureMode.HARD_FAIL
    assert "does not match documented anchor" in findings[0].message


def test_missing_vendor_tree_is_flagged(tmp_path: Path) -> None:
    """Registry references a vendor tree that doesn't exist on disk."""
    (tmp_path / "tools" / "integrity" / "docs").mkdir(parents=True, exist_ok=True)
    (tmp_path / "tools" / "integrity" / "docs" / "ground-truth-sources.md").write_text(
        "# fixture\n\n```toml\n"
        "[MissingUpstream]\n"
        'anchor_version = "1.0.0"\n'
        'anchor_sha     = "0000000000000000000000000000000000000000"\n'
        'vendor_root    = "references/MissingUpstream"\n'
        'anchor_doc     = "README.md"\n'
        'upstream_url   = "https://example.com"\n'
        'used_by_checks = ["cat1.upstream-anchor"]\n'
        "```\n"
    )
    findings = run(tmp_path)
    assert len(findings) == 1
    assert "is not present" in findings[0].message


def test_empty_registry_yields_no_findings(tmp_path: Path) -> None:
    """No registry file = no findings (anchor check is opt-in via registration)."""
    findings = run(tmp_path)
    assert findings == []
