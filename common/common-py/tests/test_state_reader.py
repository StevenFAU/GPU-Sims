"""StateReader.find_latest semantics test.

Locks in the mtime-based 'latest' semantics fixed at commit <commit SHA below>.

The old implementation picked the capture with the highest frame index,
which fails when capture dirs span multiple sessions (frame indices reset
on sim restart). The fix uses mtime so 'latest' reflects user intent:
the capture they most recently saved.
"""

from __future__ import annotations

import os
import time
from pathlib import Path

from gpusims_common.state_reader import StateReader


def test_find_latest_returns_none_for_empty_root(tmp_path: Path) -> None:
    """No capture dirs in root → None."""
    reader = StateReader(tmp_path)
    assert reader.find_latest() is None


def test_find_latest_returns_none_for_nonexistent_root(tmp_path: Path) -> None:
    """Nonexistent root dir → None (no crash)."""
    reader = StateReader(tmp_path / "does_not_exist")
    assert reader.find_latest() is None


def test_find_latest_uses_mtime_not_frame_index(tmp_path: Path) -> None:
    """When higher-frame-index capture is OLDER than lower-frame-index capture,
    find_latest must return the lower-frame one (more recent mtime).

    Reproduces the cross-session bug: capture_2301 from a previous session
    is older than capture_0319 from today; find_latest should return the
    newer one despite the lower frame index.
    """
    older_capture = tmp_path / "capture_2301"
    newer_capture = tmp_path / "capture_0319"
    older_capture.mkdir()
    newer_capture.mkdir()

    # Force mtimes: older_capture is 1 hour ago, newer_capture is now.
    one_hour_ago = time.time() - 3600
    os.utime(older_capture, (one_hour_ago, one_hour_ago))
    # newer_capture keeps its just-created mtime.

    reader = StateReader(tmp_path)
    latest = reader.find_latest()
    assert latest is not None
    assert latest.name == "capture_0319", (
        f"find_latest returned {latest.name}; expected capture_0319 "
        f"(lower frame index but more recent mtime)"
    )


def test_find_latest_ignores_non_capture_dirs(tmp_path: Path) -> None:
    """Non-capture-named dirs and files in root are skipped."""
    (tmp_path / "capture_0001").mkdir()
    (tmp_path / "not_a_capture").mkdir()
    (tmp_path / "some_file.txt").write_text("noise")

    reader = StateReader(tmp_path)
    latest = reader.find_latest()
    assert latest is not None
    assert latest.name == "capture_0001"
