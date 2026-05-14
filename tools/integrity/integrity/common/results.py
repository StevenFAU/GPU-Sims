"""Result types per spec § 3.1, § 5.4."""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from typing import Any


class FailureMode(Enum):
    HARD_FAIL = "HARD_FAIL"
    SOFT_WARN = "SOFT_WARN"
    INTERNAL_FAIL = "INTERNAL_FAIL"


@dataclass
class Finding:
    check_id: str
    mode: FailureMode
    file: str
    line: int
    message: str
    ground_truth_ref: str | None = None
    suppressed: bool = False
    suppression_reason: str | None = None
    suppression_issue: str | None = None

    def to_dict(self) -> dict[str, Any]:
        d: dict[str, Any] = {
            "check_id": self.check_id,
            "mode": self.mode.value,
            "file": self.file,
            "line": self.line,
            "message": self.message,
            "suppressed": self.suppressed,
        }
        if self.ground_truth_ref:
            d["ground_truth_ref"] = self.ground_truth_ref
        if self.suppression_reason:
            d["suppression_reason"] = self.suppression_reason
        if self.suppression_issue:
            d["suppression_issue"] = self.suppression_issue
        return d


@dataclass
class RunSummary:
    passes: int = 0
    soft_warns: int = 0
    hard_fails: int = 0
    suppressions: int = 0
