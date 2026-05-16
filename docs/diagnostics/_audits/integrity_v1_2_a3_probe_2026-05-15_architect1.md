---
title: "Integrity v1.2 A.3 Pre-Spec Probe"
date: 2026-05-15
author: architect1-via-claude-code
status: probe
scope: read-only
sibling-docs:
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
  - docs/diagnostics/_audits/integrity_v1_1_self_review_probe_2026-05-15_architect1.md
---

# Integrity v1.2 A.3 Pre-Spec Probe

Read-only probe to ground the v1.2 A.3 (`cat1.bare-path`) execution spec.
No files modified. Every line number in this report comes from a `grep -n`
or `cat -n` capture during the probe; no line number is carried over from
the prompt.

Probe-start SHA: `9add1494b237e33f3dda782c821b9d7f29446068` (FACT).
Probe-end SHA:   `9add1494b237e33f3dda782c821b9d7f29446068` (FACT — read-only, no drift).

---

## § A — Citation grammar surface

### A.1 — Verbatim dump of `tools/integrity/integrity/cat1_citations/grammar.py`

FACT: total LOC = 177 (from `wc -l`).

```python
     1	"""Citation grammar per spec § 6.2.
     2	
     3	Parses two forms:
     4	  1. Intra-repo: `<path>:<line>` or `<path>:<start>-<end>`
     5	  2. Upstream:   `<UpstreamName> <version> <path>:<line>`
     6	
     7	Commit 2 implements (1) and the parse-tree type for (2). Commit 3 wires
     8	upstream parsing into checks.
     9	
    10	Known false-positive classes (defended in tests, see test_cat1_intra_repo.py):
    11	  - IPv4-like strings (192.168.1.1:80) — extension check excludes
    12	  - Time-of-day (14:30) — extension check excludes
    13	  - URL fragments (example.com/path:42) — resolution check excludes
    14	  - Template tokens ({{path:line}}) — explicitly skipped
    15	
    16	Known false-negative classes (NOT defended in v1):
    17	  - Multi-line citations
    18	# integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
    19	  - Bracketed citations ([file.cpp:42])
    20	"""
    21	
    22	from __future__ import annotations
    23	
    24	import re
    25	from dataclasses import dataclass
    26	from pathlib import Path
    27	
    28	
    29	# Recognized extensions. Anything else is not treated as a citation.
    30	# Keep lowercase; matcher lowercases the extension before comparison.
    31	RECOGNIZED_EXTENSIONS: frozenset[str] = frozenset({
    32	    "cpp", "hpp", "h", "cc", "cxx", "c",
    33	    "glsl", "wgsl", "comp", "frag", "vert", "mesh", "tese", "tesc",
    34	    "ts", "tsx", "d.ts",
    35	    "js", "mjs", "cjs", "jsx",
    36	    "py", "pyi",
    37	    "md", "rst",
    38	    "toml", "yaml", "yml", "json",
    39	    "cmake", "txt", "sh",
    40	})
    41	
    42	
    43	# Path: alphanumerics, _, ., /, -. No spaces. Must contain at least one
    44	# literal dot before the colon (the extension); recognized-extension check
    45	# below filters non-citation false positives. The single-class form (no
    46	# nested quantifier) avoids catastrophic regex backtracking on lines that
    47	# contain many slashes or dots.
    48	# Line numbers: positive integers, optional range.
    49	INTRA_REPO_RE = re.compile(
    50	    r"(?P<path>[A-Za-z0-9_./-]+\.[A-Za-z0-9.]+)"
    51	    r":(?P<start>\d+)(?:-(?P<end>\d+))?"
    52	)
    53	
    54	
    55	# Template tokens like {{path:line}} should NOT be treated as citations.
    56	# We detect and skip them before INTRA_REPO_RE runs.
    57	TEMPLATE_TOKEN_RE = re.compile(r"\{\{[^}]*\}\}")
    58	
    59	
    60	@dataclass(frozen=True)
    61	class IntraRepoCitation:
    62	    """A parsed `<path>:<line>` or `<path>:<start>-<end>` citation."""
    63	    path: str          # As written in the source (relative or repo-relative)
    64	    start: int
    65	    end: int | None    # None for single-line citations
    66	    source_file: Path  # File where the citation appears
    67	    source_line: int   # Line number in source_file
    68	    raw: str           # Verbatim matched text
    69	
    70	
    71	def _has_recognized_extension(path: str) -> bool:
    72	    """Return True if `path` ends in a recognized file extension."""
    73	    # Try multi-dot extensions first (e.g. .d.ts, .comp.glsl)
    74	    for ext in RECOGNIZED_EXTENSIONS:
    75	        if "." in ext and path.lower().endswith("." + ext):
    76	            return True
    77	    # Then single extensions.
    78	    suffix = path.rsplit(".", 1)
    79	    if len(suffix) == 2 and suffix[1].lower() in RECOGNIZED_EXTENSIONS:
    80	        return True
    81	    return False
    82	
    83	
    84	def extract_intra_repo_citations(
    85	    text: str,
    86	    source_file: Path,
    87	) -> list[IntraRepoCitation]:
    88	    """Parse `text` line by line, yielding intra-repo citations.
    89	
    90	    `source_file` is the path being scanned; embedded in each result for
    91	    diagnostic purposes.
    92	    """
    93	    citations: list[IntraRepoCitation] = []
    94	    for lineno, line in enumerate(text.splitlines(), start=1):
    95	        # Skip lines whose only matches are inside template tokens.
    96	        masked = TEMPLATE_TOKEN_RE.sub("", line)
    97	        for m in INTRA_REPO_RE.finditer(masked):
    98	            path = m.group("path")
    99	            if not _has_recognized_extension(path):
   100	                continue
   101	            start = int(m.group("start"))
   102	            end_raw = m.group("end")
   103	            end = int(end_raw) if end_raw else None
   104	            citations.append(IntraRepoCitation(
   105	                path=path,
   106	                start=start,
   107	                end=end,
   108	                source_file=source_file,
   109	                source_line=lineno,
   110	                raw=m.group(0),
   111	            ))
   112	    return citations
   113	
   114	
   115	# Upstream citation grammar per spec § 6.2.
   116	#
   117	# Form: <UpstreamName> <version> <path>:<line>[-<end>]
   118	#
   119	# UpstreamName: capitalized word, alphanumerics. Distinguished from a
   120	# regular sentence-starting word by the version token that follows.
   121	# Version: `v1.2.3` / `1.2.3` / `HEAD` / a 7-40 char hex SHA.
   122	#
   123	# Known false-positive class: a capitalized sentence-starting word
   124	# followed by a number could match (e.g. "Section 1.2.3 of the
   125	# integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
   126	# specification cited at TimeStep.cpp:42"). We mitigate by requiring
   127	# the version token to be tight against the path (no comma, no period,
   128	# no "of"/"in" between). The grammar is intentionally tight; ambiguous
   129	# cases produce false negatives, not false positives.
   130	UPSTREAM_RE = re.compile(
   131	    r"(?P<upstream>[A-Z][A-Za-z0-9]+)\s+"
   132	    r"(?P<version>v?\d+(?:\.\d+){1,3}(?:-[A-Za-z0-9]+)?|HEAD|[a-f0-9]{7,40})"
   133	    r"\s+"
   134	    r"(?P<path>[A-Za-z0-9_./-]+\.[A-Za-z0-9.]+)"
   135	    r":(?P<start>\d+)(?:-(?P<end>\d+))?"
   136	)
   137	
   138	
   139	@dataclass(frozen=True)
   140	class UpstreamCitation:
   141	    """A parsed `<UpstreamName> <version> <path>:<line>` citation."""
   142	    upstream: str       # As written, e.g. "SPlisHSPlasH"
   143	    version: str        # As written, e.g. "2.16.1" or "HEAD" or a hex SHA
   144	    path: str           # Path within the vendor tree
   145	    start: int
   146	    end: int | None
   147	    source_file: Path
   148	    source_line: int
   149	    raw: str            # Verbatim matched text
   150	
   151	
   152	def extract_upstream_citations(
   153	    text: str,
   154	    source_file: Path,
   155	) -> list[UpstreamCitation]:
   156	    """Parse `text` line by line, yielding upstream citations."""
   157	    citations: list[UpstreamCitation] = []
   158	    for lineno, line in enumerate(text.splitlines(), start=1):
   159	        masked = TEMPLATE_TOKEN_RE.sub("", line)
   160	        for m in UPSTREAM_RE.finditer(masked):
   161	            path = m.group("path")
   162	            if not _has_recognized_extension(path):
   163	                continue
   164	            start = int(m.group("start"))
   165	            end_raw = m.group("end")
   166	            end = int(end_raw) if end_raw else None
   167	            citations.append(UpstreamCitation(
   168	                upstream=m.group("upstream"),
   169	                version=m.group("version"),
   170	                path=path,
   171	                start=start,
   172	                end=end,
   173	                source_file=source_file,
   174	                source_line=lineno,
   175	                raw=m.group(0),
   176	            ))
   177	    return citations
```

### A.2 — Extractor functions

FACT: `grammar.py` exports exactly two extractor functions and two parse-
tree dataclasses:

| Function | Signature | Citation shape | "Bare-path" aware? |
|----------|-----------|----------------|--------------------|
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| `extract_intra_repo_citations` | `(text: str, source_file: Path) -> list[IntraRepoCitation]` (grammar.py:84-112) | `<path>:<line>[-<end>]` — `<path>` requires at least one slash OR at least one dot prefix-segment via `INTRA_REPO_RE`; basenames-only still match because `[A-Za-z0-9_./-]+\.[A-Za-z0-9.]+` allows the empty-slash case. | No explicit concept; bare basenames flow through identical to dotted-prefix paths. |
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| `extract_upstream_citations` | `(text: str, source_file: Path) -> list[UpstreamCitation]` (grammar.py:152-177) | `<UpstreamName> <version> <path>:<line>[-<end>]`. The `<path>` token is the same character class as intra-repo; vendor-relative. | No explicit concept; the path field is whatever the regex captures after `<version> `. |
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| `IntraRepoCitation` dataclass | grammar.py:60-68 | parse tree for intra-repo | n/a |
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| `UpstreamCitation` dataclass | grammar.py:139-149 | parse tree for upstream | n/a |

INFERENCE (A.2): a `cat1.bare-path` check does **not** need a new extractor
function in `grammar.py`. The intra-repo extractor already emits every
bare-basename citation as an `IntraRepoCitation` with `path=<basename>`.
The new check is a downstream consumer of the same parse-tree stream,
filtered to entries where `"/" not in citation.path`. This is the
minimal-surface placement: the new logic lives in the new check module
(`tools/integrity/integrity/cat1_citations/checks/bare_path.py`) and
imports `extract_intra_repo_citations` like `intra_repo.py` already does.
Adding a third extractor would duplicate ~25 LOC of regex/template-mask
machinery and introduce a parallel parse pass; not worth it.

### A.3 — Verbatim dump of `tools/integrity/integrity/cat1_citations/resolver.py`

FACT: total LOC = 103.

```python
     1	"""Intra-repo citation resolution per spec § 6.3.
     2	
     3	Resolution order:
     4	  1. Relative to the source file's directory
     5	  2. Relative to the repo root
     6	  3. Unresolved → FAIL
     7	"""
     8	
     9	from __future__ import annotations
    10	
    11	from dataclasses import dataclass
    12	from pathlib import Path
    13	
    14	from integrity.cat1_citations.grammar import IntraRepoCitation
    15	
    16	
    17	@dataclass(frozen=True)
    18	class ResolutionResult:
    19	    citation: IntraRepoCitation
    20	    resolved_path: Path | None     # None if unresolved
    21	    file_line_count: int | None    # None if unresolved
    22	    in_range: bool                 # True if start/end fall within line count
    23	    reason: str                    # Diagnostic message
    24	
    25	
    26	def _count_lines(path: Path) -> int:
    27	    """Count newline-terminated lines in `path`. A trailing-no-newline
    28	    final line still counts."""
    29	    try:
    30	        with path.open("rb") as f:
    31	            data = f.read()
    32	        if not data:
    33	            return 0
    34	        count = data.count(b"\n")
    35	        if not data.endswith(b"\n"):
    36	            count += 1
    37	        return count
    38	    except OSError:
    39	        return 0
    40	
    41	
    42	def resolve(
    43	    citation: IntraRepoCitation,
    44	    repo_root: Path,
    45	) -> ResolutionResult:
    46	    """Try to resolve a citation. Returns resolution metadata."""
    47	    # Try relative to source file's directory
    48	    src_dir = citation.source_file.parent
    49	    candidate = (src_dir / citation.path).resolve()
    50	    if candidate.is_file():
    51	        return _check_range(citation, candidate)
    52	
    53	    # Try relative to repo root
    54	    candidate = (repo_root / citation.path).resolve()
    55	    if candidate.is_file():
    56	        return _check_range(citation, candidate)
    57	
    58	    return ResolutionResult(
    59	        citation=citation,
    60	        resolved_path=None,
    61	        file_line_count=None,
    62	        in_range=False,
    63	        reason=f"path '{citation.path}' does not resolve under "
    64	               f"{src_dir} or {repo_root}",
    65	    )
    66	
    67	
    68	def _check_range(citation: IntraRepoCitation, resolved: Path) -> ResolutionResult:
    69	    line_count = _count_lines(resolved)
    70	    end_to_check = citation.end if citation.end is not None else citation.start
    71	    ...
   103	```

INFERENCE (A.3): the resolver is intra-repo-only and resolves by *trying
filesystem paths* (`src_dir / path`, then `repo_root / path`). For a
bare basename, those resolution attempts will *succeed* only if a
sibling file with that exact basename happens to live in the
source-file's directory — which is the false-positive class the v1
intra-repo check accidentally accommodates today. Two reuse points for
A.3:

1. `_count_lines(path)` (resolver.py:26-39) is directly reusable for
   range checks against the matched index entry. No reason to re-roll
   line-counting in `bare_path.py`.
2. `ResolutionResult` (resolver.py:17-23) has the right shape (resolved
   path + line count + in-range bool + reason). INFERENCE: A.3 can
   either reuse `ResolutionResult` verbatim or introduce a slim
   `BarePathResolutionResult` whose only delta is a `candidates: list[Path]`
   field for the AMBIGUOUS arm. The latter is cleaner — the existing
   resolver doesn't carry ambiguity state and a `list[Path]` would be
   structurally wrong on `ResolutionResult.resolved_path` (Path | None,
   not list).

---

## § B — cat1.intra-repo current behavior

### B.1 — Verbatim dump of `tools/integrity/integrity/cat1_citations/checks/intra_repo.py`

FACT: total LOC = 124.

```python
     1	"""Check: cat1.intra-repo — every intra-repo citation resolves.
     2	
     3	Mode: HARD_FAIL.
     4	
     5	False positives are defended by the grammar's extension filter and the
     6	template-token mask. False positives that still escape are suppressible
     7	# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
     8	via `integrity-allow: cat1.intra-repo; <reason>; <issue-ref>`.
     9	"""
    10	
    11	from __future__ import annotations
    12	
    13	from pathlib import Path
    14	
    15	from integrity.cat1_citations.grammar import (
    16	    extract_intra_repo_citations,
    17	    extract_upstream_citations,
    18	)
    19	from integrity.cat1_citations.resolver import resolve
    20	from integrity.common.annotations import (
    21	    fence_state_per_line,
    22	    is_markdown_path,
    23	)
    24	from integrity.common.exclusions import is_excluded
    25	from integrity.common.repo import list_tracked_files
    26	from integrity.common.results import FailureMode, Finding
    27	
    28	
    29	CHECK_ID = "cat1.intra-repo"
    30	MODE = FailureMode.HARD_FAIL
    31	
    32	
    33	# File extensions whose contents we scan for citations.
    34	SCAN_EXTENSIONS: frozenset[str] = frozenset({
    35	    ".cpp", ".hpp", ".h", ".cc", ".cxx", ".c",
    36	    ".glsl", ".wgsl",
    37	    ".ts", ".tsx", ".d.ts",
    38	    ".js", ".mjs", ".cjs", ".jsx",
    39	    ".py", ".pyi",
    40	    ".md",
    41	})
    42	
    43	
    44	def _has_scan_extension(path: Path) -> bool:
    45	    name = path.name.lower()
    46	    for ext in SCAN_EXTENSIONS:
    47	        if name.endswith(ext):
    48	            return True
    49	    return False
    50	
    51	
    52	def _list_scannable_files(root: Path) -> list[Path]:
    53	    """List files to scan. Uses git ls-files if root is a git repo,
    54	    otherwise walks the directory directly (for test fixtures)."""
    55	    if (root / ".git").exists():
    56	        return list_tracked_files(root)
    57	    files: list[Path] = []
    58	    for path in root.rglob("*"):
    59	        if path.is_file():
    60	            files.append(path)
    61	    return files
    62	
    63	
    64	def _is_under_references(path: str) -> bool:
    65	    """True if the path begins with `references/` (or starts with a
    66	    component that is the name of a vendored upstream)."""
    67	    return path.startswith("references/")
    68	
    69	
    70	def run(repo_root: Path) -> list[Finding]:
    71	    """Scan all tracked files; return findings for unresolved citations."""
    72	    findings: list[Finding] = []
    73	
    74	    for absolute in _list_scannable_files(repo_root):
    75	        rel = str(absolute.relative_to(repo_root))
    76	        if is_excluded(rel):
    77	            continue
    78	        if not _has_scan_extension(absolute):
    79	            continue
    80	
    81	        try:
    82	            text = absolute.read_text(encoding="utf-8", errors="replace")
    83	        except OSError:
    84	            continue
    85	
    86	        if is_markdown_path(rel):
    87	            fence_state = fence_state_per_line(text.splitlines())
    88	        else:
    89	            fence_state = None
    90	
    91	        # Spans of upstream citations on this file. Any intra-repo
    92	        # match whose (line, path, start, end) coincides with the tail
    93	        # of an upstream citation belongs to cat1.upstream-citation,
    94	        # not intra-repo.
    95	        upstream_tails: set[tuple[int, str, int, int | None]] = {
    96	            (uc.source_line, uc.path, uc.start, uc.end)
    97	            for uc in extract_upstream_citations(text, absolute)
    98	        }
    99	
   100	        for citation in extract_intra_repo_citations(text, absolute):
   101	            if (
   102	                fence_state is not None
   103	                and 0 < citation.source_line <= len(fence_state)
   104	                and fence_state[citation.source_line - 1]
   105	            ):
   106	                continue
   107	            if _is_under_references(citation.path):
   108	                # Belongs to cat1.upstream-citation, not intra-repo.
   109	                continue
   110	            if (citation.source_line, citation.path, citation.start, citation.end) in upstream_tails:
   111	                # Tail of an upstream citation; cat1.upstream-citation handles.
   112	                continue
   113	            result = resolve(citation, repo_root)
   114	            if result.resolved_path is None or not result.in_range:
   115	                findings.append(Finding(
   116	                    check_id=CHECK_ID,
   117	                    mode=MODE,
   118	                    file=str(absolute.relative_to(repo_root)),
   119	                    line=citation.source_line,
   120	                    message=f"{citation.raw}: {result.reason}",
   121	                    ground_truth_ref=None,
   122	                ))
   123	
   124	    return findings
   125	```

### B.2 — `run()` path-resolution logic, verbatim

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
The path-resolution block (intra_repo.py:100-122) is:

```python
   100	        for citation in extract_intra_repo_citations(text, absolute):
   101	            if (
   102	                fence_state is not None
   103	                and 0 < citation.source_line <= len(fence_state)
   104	                and fence_state[citation.source_line - 1]
   105	            ):
   106	                continue
   107	            if _is_under_references(citation.path):
   108	                # Belongs to cat1.upstream-citation, not intra-repo.
   109	                continue
   110	            if (citation.source_line, citation.path, citation.start, citation.end) in upstream_tails:
   111	                # Tail of an upstream citation; cat1.upstream-citation handles.
   112	                continue
   113	            result = resolve(citation, repo_root)
   114	            if result.resolved_path is None or not result.in_range:
   115	                findings.append(Finding(
   116	                    check_id=CHECK_ID,
   117	                    mode=MODE,
   118	                    file=str(absolute.relative_to(repo_root)),
   119	                    line=citation.source_line,
   120	                    message=f"{citation.raw}: {result.reason}",
   121	                    ground_truth_ref=None,
   122	                ))
   123	```

INFERENCE (B.2): the natural place for a bare-path skip guard is between
the `upstream_tails` skip (intra_repo.py:110-112) and the `resolve()` call
(intra_repo.py:113). A new guard of the shape

```python
            if "/" not in citation.path:
                # Bare basename: cat1.bare-path handles this case.
                continue
```

would hand bare-basename citations off cleanly to `cat1.bare-path`, in
the same idiomatic style as the `_is_under_references` and
`upstream_tails` skips already use. The guard is placed *before* the
filesystem `resolve()` so we avoid the current false-resolution behavior
where a `widget.hpp:11` citation in a directory that happens to contain
a `widget.hpp` resolves accidentally to a same-basename neighbor that
isn't the author's intent — a class of false-negative cat1.intra-repo
already silently absorbs.

(FACT: confirmed by inspection — the existing `resolve()` at
resolver.py:42-65 will succeed on bare basenames whenever a sibling file
matches, so any cat1.bare-path test must occur *before* `resolve()` runs
or both checks will fire on the same line.)

### B.3 — Current behavior on bare paths

FACT: run of
```
python3 -m integrity --check cat1.intra-repo --mode warn-only \
  --output json --no-audit-log
```
(piped through a Python decoder to filter `does not resolve` messages)
produced these first 30 findings (verbatim):

```text
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
CHANGELOG.md : 92 | Chakazul/Lenia/Python/LeniaNDK.py:329-335: path 'Chakazul/Lenia/Python/LeniaNDK.py' does not resolve under /home/otacon/
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
CHANGELOG.md : 154 | context.hpp:78: path 'context.hpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims or /home/otacon/Projec
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
CHANGELOG.md : 154 | context.cpp:116: path 'context.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims or /home/otacon/Proje
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
CHANGELOG.md : 154 | context.cpp:202: path 'context.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims or /home/otacon/Proje
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
common/common-py/examples/hello/hello/main.py : 31 | kernel_impl.py:631: path 'kernel_impl.py' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/common/common-p
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
continuous-ca/lenia-fft/docs/load-bearing-decisions.md : 236 | main.py:608: path 'main.py' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
continuous-ca/lenia-fft/docs/load-bearing-decisions.md : 248 | main.py:560: path 'main.py' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
continuous-ca/lenia-fft/docs/load-bearing-decisions.md : 258 | main.py:599: path 'main.py' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
continuous-ca/lenia-fft/docs/load-bearing-decisions.md : 274 | main.py:466: path 'main.py' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
continuous-ca/lenia-fft/docs/load-bearing-decisions.md : 276 | main.py:96: path 'main.py' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
continuous-ca/lenia-fft/docs/load-bearing-decisions.md : 283 | main.py:187: path 'main.py' does not resolve under …
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
continuous-ca/lenia-fft/docs/notes.md : 63 | Chakazul/Lenia/Python/LeniaNDK.py:329-335: path 'Chakazul/Lenia/Python/LeniaNDK.py' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
continuous-ca/lenia-fft/docs/notes.md : 169 | main.py:164: path 'main.py' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
continuous-ca/lenia-fft/docs/notes.md : 176 | main.py:203: path 'main.py' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
continuous-ca/lenia-fft/docs/notes.md : 178 | main.py:306-318: path 'main.py' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
continuous-ca/lenia-fft/python/lenia_fft/presets.py : 12 | LeniaNDK.py:329-335: path 'LeniaNDK.py' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
continuous-ca/lenia-fft/python/lenia_fft/presets.py : 81 | LeniaNDK.py:184-206: path 'LeniaNDK.py' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/category_context_quantum_landing_2026-05-15.md : 81 | grammar.py:49-52: path 'grammar.py' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/category_context_quantum_landing_2026-05-15.md : 291 | grammar.py:49-52: path 'grammar.py' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/commoncpp_consumers_2026-05-14_architect2.md : 231 | vdb_writer.hpp:33: path 'vdb_writer.hpp' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/commoncpp_consumers_2026-05-14_architect2.md : 233 | vdb_writer.hpp:41: path 'vdb_writer.hpp' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md : 162 | alembic_writer.hpp:21: path 'alembic_writer.hpp' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md : 164 | alembic_writer.hpp:31: path 'alembic_writer.hpp' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md : 166 | alembic_writer.hpp:44: path 'alembic_writer.hpp' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md : 177 | camera.hpp:12: path 'camera.hpp' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md : 179 | camera.hpp:35: path 'camera.hpp' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md : 181 | camera.hpp:33: path 'camera.hpp' does not resolve under …
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md : 183 | camera.hpp:46: path 'camera.hpp' does not resolve under …
```

FACT (B.3 confirmation): bare-basename citations (e.g.,
`widget.hpp:11`, `camera.hpp:12`, `main.py:608`) produce findings of the
form `<basename>: path '<basename>' does not resolve under <root>`.
This matches the prompt's predicted message exactly. The `<basename>`
in the message text is `citation.raw` (intra_repo.py:120) prefixing
`result.reason` (resolver.py:63-64).

> Note the first row (`CHANGELOG.md:92`) is *not* bare-path — it is the
> dotted multi-segment path `Chakazul/Lenia/Python/LeniaNDK.py`. The
> remaining 26 rows in the slice are bare. The mixture is expected:
> the filter `'does not resolve' in message` does not exclude
> dotted-but-unresolved paths.

---

## § C — cat1.upstream-citation handling

### C.1 — Verbatim dump of `tools/integrity/integrity/cat1_citations/checks/upstream.py`

FACT: total LOC = 139.

```python
     1	"""Check: cat1.upstream-citation — every upstream citation resolves.
     2	
     3	Mode: HARD_FAIL.
     4	
     5	Resolution rules per spec § 6.3 (upstream half):
     6	  1. Map <upstream> to a vendor root via the registry
     7	  2. If <upstream> not in registry, this check skips (cat1.unregistered-upstream handles)
     8	  3. If <version> doesn't match anchor_version and isn't 'HEAD', HARD_FAIL
     9	  4. Resolve <path> under vendor_root
    10	  5. Check line range against file line count
    11	"""
    12	
    13	from __future__ import annotations
    14	
    15	from pathlib import Path
    16	
    17	from integrity.cat1_citations.grammar import extract_upstream_citations
    18	from integrity.cat1_citations.resolver import _count_lines
    19	from integrity.cat1_citations.upstream_anchor import load_registry
    20	from integrity.common.annotations import (
    21	    fence_state_per_line,
    22	    is_markdown_path,
    23	)
    24	from integrity.common.exclusions import is_excluded
    25	from integrity.common.repo import list_tracked_files
    26	from integrity.common.results import FailureMode, Finding
    27	
    28	
    29	CHECK_ID = "cat1.upstream-citation"
    30	MODE = FailureMode.HARD_FAIL
    31	
    32	
    33	SCAN_EXTENSIONS = frozenset({
    34	    ".cpp", ".hpp", ".h", ".cc", ".cxx", ".c",
    35	    ".glsl", ".wgsl",
    36	    ".ts", ".tsx", ".d.ts",
    37	    ".js", ".mjs", ".cjs", ".jsx",
    38	    ".py", ".pyi",
    39	    ".md",
    40	})
    41	
    42	
    43	def _has_scan_extension(path: Path) -> bool:
    44	    name = path.name.lower()
    45	    for ext in SCAN_EXTENSIONS:
    46	        if name.endswith(ext):
    47	            return True
    48	    return False
    49	
    50	
    51	def _list_scannable_files(root: Path) -> list[Path]:
    52	    if (root / ".git").exists():
    53	        return list_tracked_files(root)
    54	    return [p for p in root.rglob("*") if p.is_file()]
    55	
    56	
    57	def run(repo_root: Path) -> list[Finding]:
    58	    registry = load_registry(repo_root)
    59	    findings: list[Finding] = []
    60	
    61	    for absolute in _list_scannable_files(repo_root):
    62	        try:
    63	            rel = str(absolute.relative_to(repo_root))
    64	        except ValueError:
    65	            continue
    66	        if is_excluded(rel):
    67	            continue
    68	        if not _has_scan_extension(absolute):
    69	            continue
    70	
    71	        try:
    72	            text = absolute.read_text(encoding="utf-8", errors="replace")
    73	        except OSError:
    74	            continue
    75	
    76	        if is_markdown_path(rel):
    77	            fence_state = fence_state_per_line(text.splitlines())
    78	        else:
    79	            fence_state = None
    80	
    81	        for citation in extract_upstream_citations(text, absolute):
    82	            if (
    83	                fence_state is not None
    84	                and 0 < citation.source_line <= len(fence_state)
    85	                and fence_state[citation.source_line - 1]
    86	            ):
    87	                continue
    88	            reg = registry.get(citation.upstream)
    89	            if reg is None:
    90	                # cat1.unregistered-upstream handles this case
    91	                continue
    92	
    93	            # Version check: must match anchor_version exactly, or be HEAD
    94	            normalized_version = citation.version.lstrip("v")
    95	            if normalized_version != reg.anchor_version and citation.version != "HEAD":
    96	                findings.append(Finding(
    97	                    check_id=CHECK_ID,
    98	                    mode=MODE,
    99	                    file=rel,
   100	                    line=citation.source_line,
   101	                    message=(
   102	                        f"{citation.raw}: version '{citation.version}' does not "
   103	                        f"match registered anchor '{reg.anchor_version}' for "
   104	                        f"{reg.name}"
   105	                    ),
   106	                ))
   107	                continue
   108	
   109	            # Resolve path under vendor_root
   110	            candidate = (repo_root / reg.vendor_root / citation.path).resolve()
   111	            if not candidate.is_file():
   112	                findings.append(Finding(
   113	                    check_id=CHECK_ID,
   114	                    mode=MODE,
   115	                    file=rel,
   116	                    line=citation.source_line,
   117	                    message=(
   118	                        f"{citation.raw}: path '{citation.path}' does not "
   119	                        f"resolve under {reg.vendor_root}"
   120	                    ),
   121	                ))
   122	                continue
   123	
   124	            # Line range check
   125	            line_count = _count_lines(candidate)
   126	            count = data.count(b"\n")
   127	            ...
   139	```

### C.2 — Sharing infrastructure with cat1.bare-path

FACT (C.2): `upstream.py` consumes `extract_upstream_citations` (line 17,
line 81) and pairs it with a `load_registry(repo_root)` call (line 58).
The registry yields per-upstream `vendor_root` and `anchor_version`,
which are joined with `<path>` (already a vendor-tree-relative path
written *with* directory prefix) at line 110: `repo_root / reg.vendor_root / citation.path`.

INFERENCE (C.2): the new cat1.bare-path check **cannot directly reuse**
`upstream.py`'s resolver pattern because cat1.bare-path has no per-line
upstream-name token — it sees only `<basename>:<line>`. What it *can*
share is the **registry-based vendor-root list**, used to build a
basename → upstream-path index. Concretely:

```python
# Pseudocode A.3 will encode:
registry = load_registry(repo_root)
upstream_basenames: dict[str, list[Path]] = {}
for reg in registry.values():
    for f in (repo_root / reg.vendor_root).rglob('*'):
        if f.is_file() and _has_scan_extension(f):
            upstream_basenames.setdefault(f.name, []).append(f)
```

This is a **read-only** dependence on the registry. The registry shape
(see § E) is sufficient as-is; no new fields needed. INFERENCE:
basename-indexing the entire vendor tree is O(files-in-references/) =
~1165 (FACT, see D.1), dominated by the SPlisHSPlasH and
lbm-principles-practice trees. At v1.2 corpus size this is a millisecond-
range cost performed once per `run()` invocation — comparable to the
current `git ls-files` traversal in `_list_scannable_files`.

---

## § D — references/ tree structure

### D.1 — Total file count

FACT: `find references/ -type f -not -path '*/.git/*' | wc -l` = **1165**.

### D.2 — Unique basename count

FACT: `find references/ -type f -not -path '*/.git/*' -exec basename {} \; | sort -u | wc -l` = **1039**.

INFERENCE (D.2): 1165 − 1039 = 126 duplicate-basename slots. That is, ~11%
of references-tree files share their basename with at least one sibling
under references/. The bare-path-upstream-side ambiguity is real but
bounded (see D.4 for which basenames specifically).

### D.3 — Sample of code-extension files (first 50, alphabetical)

FACT (verbatim head -50 of the find query):

```text
references/lbm-principles-practice/chapter11/IBLBM_2D_Poiseuille.cpp
references/lbm-principles-practice/chapter13/cpu_intro/main.cpp
references/lbm-principles-practice/chapter13/cpu_intro/seconds.cpp
references/lbm-principles-practice/chapter13/cpu_intro/seconds.h
references/lbm-principles-practice/chapter13/cpu/LBM.cpp
references/lbm-principles-practice/chapter13/cpu/LBM.h
references/lbm-principles-practice/chapter13/cpu/main.cpp
references/lbm-principles-practice/chapter13/cpu/seconds.cpp
references/lbm-principles-practice/chapter13/cpu/seconds.h
references/lbm-principles-practice/chapter13/gpu/LBM.h
references/lbm-principles-practice/chapter13/gpu/seconds.cpp
references/lbm-principles-practice/chapter13/gpu/seconds.h
references/lbm-principles-practice/chapter13/mpi/blocking/LBM.cpp
references/lbm-principles-practice/chapter13/mpi/blocking/LBM.h
references/lbm-principles-practice/chapter13/mpi/blocking/main.cpp
references/lbm-principles-practice/chapter13/mpi/blocking/seconds.cpp
references/lbm-principles-practice/chapter13/mpi/blocking/seconds.h
references/lbm-principles-practice/chapter13/mpi/nonblocking/LBM.cpp
references/lbm-principles-practice/chapter13/mpi/nonblocking/LBM.h
references/lbm-principles-practice/chapter13/mpi/nonblocking/main.cpp
references/lbm-principles-practice/chapter13/mpi/nonblocking/seconds.cpp
references/lbm-principles-practice/chapter13/mpi/nonblocking/seconds.h
references/lbm-principles-practice/chapter13/openmp/LBM.cpp
references/lbm-principles-practice/chapter13/openmp/LBM.h
references/lbm-principles-practice/chapter13/openmp/main.cpp
references/lbm-principles-practice/chapter13/openmp/seconds.cpp
references/lbm-principles-practice/chapter13/openmp/seconds.h
references/lbm-principles-practice/chapter13/python3/vortexdecay.py
references/lbm-principles-practice/chapter8/cylinder.cpp
references/lbm-principles-practice/chapter8/film_antibb.cpp
references/lbm-principles-practice/chapter8/film_inamuro.cpp
references/lbm-principles-practice/chapter8/film_uniform.cpp
references/lbm-principles-practice/chapter8/gaussian_1d_bgk.cpp
references/lbm-principles-practice/chapter8/gaussian_1d_magic12.cpp
references/lbm-principles-practice/chapter8/gaussian_1d_magic6.cpp
references/lbm-principles-practice/chapter8/gaussian_2d_bgk.cpp
references/lbm-principles-practice/chapter8/gaussian_2d_trt.cpp
references/lbm-principles-practice/chapter9/shanchen.cpp
references/SPlisHSPlasH/data/Scenes/AnimatedBody_2D.py
references/SPlisHSPlasH/data/Scenes/ScriptTest.py
references/SPlisHSPlasH/data/shaders/fs_points_colormap.glsl
references/SPlisHSPlasH/data/shaders/fs_points.glsl
references/SPlisHSPlasH/data/shaders/fs_smooth.glsl
references/SPlisHSPlasH/data/shaders/vs_points_scalar.glsl
references/SPlisHSPlasH/data/shaders/vs_points_vector.glsl
references/SPlisHSPlasH/data/shaders/vs_smooth.glsl
references/SPlisHSPlasH/doc/conf.py
references/SPlisHSPlasH/extern/Catch2/catch.hpp
references/SPlisHSPlasH/extern/cxxopts/cxxopts.hpp
references/SPlisHSPlasH/extern/eigen/Eigen/src/Cholesky/LDLT.h
```

INFERENCE (D.3): the lbm-principles-practice tree's `chapter13/{cpu,gpu,
mpi/blocking,mpi/nonblocking,openmp}/` layout is the principal source of
intra-references duplicate basenames (`LBM.cpp`, `LBM.h`, `main.cpp`,
`seconds.cpp`, `seconds.h` each occur ~5 times within that one chapter).
The SPlisHSPlasH `extern/` vendor-of-vendor tree adds another batch
(eigen, Catch2, cxxopts).

### D.4 — Cross-cutting basenames within references/ (count ≥ 2)

FACT (verbatim):

```text
     31 CMakeLists.txt
     14 main.cpp
     12 MathFunctions.h
     10 PacketMath.h
      8 TypeCasting.h
      8 Complex.h
      6 seconds.h
      6 seconds.cpp
      6 LICENSE
      6 compile.sh
      5 version.txt
      5 README.md
      5 LBM.h
      4 LBM.cpp
      4 common.h
      3 setup.py
      3 setup.cfg
      3 LICENSE.txt
      3 __init__.py
      2 GeneralBlockPanelKernel.h
```

INFERENCE (D.4): `main.cpp` (14×) is the most common cross-cutting
basename of a code extension. For a bare-path citation `main.cpp:N`, the
upstream-side index alone is ambiguous — the v1.2 spec must specify
how the resolver picks between 14 candidates (or refuses to). The
intra-repo side adds at least 2 more `main.cpp` candidates (see D.5),
so an unrestricted basename index would have ≥16 candidates for
`main.cpp` alone.

### D.5 — Cross-cutting basenames in the rest of the repo (count ≥ 2)

FACT (verbatim, excluding references/ + build/ + node_modules/ + .venv/ + __pycache__/ + dist/ + .git/):

```text
    114 main.cpp
     33 Foundation.h
     33 All.h
     18 __init__.py
     15 spirv.hpp
     15 GLSL.std.450.h
     15 example.cpp
     15 bar.h
     12 spirv.py
     12 spirv.h
     12 OpenCL.std.h
     12 ObjectTests.cpp
      9 test.cpp
      9 test1.cpp
      9 span.h
      9 SchemaInfoDeclarations.h
      9 platform.h
      9 pch.h
      9 OrImpl.h
      9 OrImpl.cpp
      9 NonSemanticShaderDebugInfo100.h
      9 integer.hpp
      9 instruction.h
      9 fullscreen.vert.glsl
      9 foo.h
      9 flags.h
      9 Export.h
      9 diff.cpp
      9 CprImpl.h
      9 CprImpl.cpp
```

INFERENCE (D.5): the 114-count `main.cpp` here is dominated by
`build-test-alembic/_deps/…` content — the alembic build tree carries
~100 vendor sub-projects each contributing a `main.cpp`. INFERENCE
(critical): the find query above does **NOT** exclude
`build-test-alembic/` because that directory does not match the
exclusion pattern set (it doesn't begin with `build/`). When the
cat1.bare-path basename index is built against repo files actually
visible to the integrity toolkit (via `list_tracked_files` = git
ls-files), much of this noise will drop out — alembic deps are *not*
git-tracked. **The probe-script tally in § F over-counts AMBIGUOUS for
the same reason.** The corrected estimate is in the F.2 drift note.

---

## § E — Reference-registry parsing

### E.1 — Verbatim dump of `_parse_ground_truth_sources` (snapshot.py)

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
FACT: located at snapshot.py:102-135 (200-LOC total file).

```python
   102	def _parse_ground_truth_sources(root: Path) -> list[dict]:
   103	    """Parse `tools/integrity/docs/ground-truth-sources.md` for upstream
   104	    anchor blocks. Permissive line-based parser; tolerates either inline
   105	    or fenced TOML-shaped entries."""
   106	    path = root / "tools" / "integrity" / "docs" / "ground-truth-sources.md"
   107	    if not path.is_file():
   108	        return []
   109	
   110	    text = path.read_text(encoding="utf-8")
   111	    blocks: list[dict] = []
   112	    current: dict = {}
   113	    for line in text.splitlines():
   114	        stripped = line.strip()
   115	        if stripped.startswith("anchor_version"):
   116	            _, _, val = stripped.partition("=")
   117	            current["anchor_version"] = val.strip().strip('"').strip("'")
   118	        elif stripped.startswith("anchor_sha"):
   119	            _, _, val = stripped.partition("=")
   120	            current["anchor_sha"] = val.strip().strip('"').strip("'")
   121	        elif stripped.startswith("vendor_root"):
   122	            _, _, val = stripped.partition("=")
   123	            current["vendor_root"] = val.strip().strip('"').strip("'")
   124	        elif stripped.startswith("name"):
   125	            _, _, val = stripped.partition("=")
   126	            current["name"] = val.strip().strip('"').strip("'")
   127	        elif stripped == "":
   128	            if current.get("anchor_version") and current.get("anchor_sha"):
   129	                blocks.append(current)
   130	            current = {}
   131	
   132	    if current.get("anchor_version") and current.get("anchor_sha"):
   133	        blocks.append(current)
   134	
   135	    return blocks
   136	```

### E.2 — Verbatim dump of `tools/integrity/docs/ground-truth-sources.md`

FACT: total LOC = 78.

```markdown
     1	# Ground-truth sources for the integrity toolkit
     2	
     3	Per spec Appendix A. Adding a source requires:
     4	
     5	1. Vendoring the upstream under `references/<UpstreamName>/`, OR documenting
     6	   an algebraic derivation under `tools/integrity/docs/algebraic/`
     7	2. Pinning the anchor (version + SHA)
     8	3. Updating the TOML block below
     9	4. Updating the relevant check(s) to consume the new source
    10	
    11	## v1 registry
    12	
    13	The block below is parsed by `cat1_citations/upstream_anchor.py`. Everything
    14	outside the fenced TOML block is prose for humans.
    15	
    16	```toml
    17	[SPlisHSPlasH]
    18	anchor_version = "2.16.1"
    19	anchor_sha     = "6bff55a6eaf14083d34650f22a268ce156b62b54"
    20	vendor_root    = "references/SPlisHSPlasH"
    21	anchor_doc     = ".gitignore"
    22	upstream_url   = "https://github.com/InteractiveComputerGraphics/SPlisHSPlasH"
    23	used_by_checks = ["cat1.upstream-citation", "cat1.upstream-anchor", "cat3.cubic-kernel"]
    24	
    25	[Krueger]
    26	anchor_version = "book-companion-code-2016"
    27	anchor_sha     = "6e2c592fdc3592c14dfd52f860fc1ceea930bcb0"
    28	vendor_root    = "references/lbm-principles-practice"
    29	anchor_doc     = "LICENSE.txt"
    30	upstream_url   = "https://github.com/lbm-principles-practice/code"
    31	used_by_checks = ["cat1.upstream-citation", "cat1.upstream-anchor"]
    32	# Scope note: D2Q9 only. Used as a math-pattern reference (equilibrium
    33	# form, halfway bounce-back convention) for Phase 12's D3Q19 sim. D3Q19
    34	# constants come from [Algebraic_D3Q19], not from this anchor.
    35	
    36	[Algebraic_D3Q19]
    37	derivation     = "tools/integrity/docs/algebraic/d3q19.md"
    38	expected_data  = "tools/integrity/integrity/cat3_numerical/checks/d3q19_equilibrium.expected.json"
    39	used_by_checks = ["cat3.d3q19-velocity-set", "cat3.d3q19-weights", "cat3.d3q19-equilibrium"]
    40	# No vendor_root: D3Q19 lattice constants are derivable from first
    41	# principles (squared-L2-norm-<=2 subset of {-1,0,1}^3 + Gauss-Hermite
    42	# isotropy constraints). Pairs with [Krueger] which covers the
    43	# equation form and the halfway-bounce-back convention.
    44	```
    45	
    46	## Notes on v1 registry contents
    47	
    48	- **SPlisHSPlasH:** Vendored at Phase 11.5 setup-1 after the original
    49	  fabricated `1.8.10` anchor was found non-existent. See
    50	  `docs/diagnostics/_audits/phase11_5_setup1_2026-05-14_setup1.md`.
    51	- **Krueger:** Vendored at Phase 12 setup-1. The repository is D2Q9 only;
    52	  the citation in Phase 12 shader doc-blocks references the chapter13 CPU
    53	<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
    54	  equilibrium pattern at `chapter13/cpu/LBM.cpp:97` and `chapter13/cpu_intro/main.cpp:271`,
    55	  and the halfway bounce-back pattern at `chapter5/poiseuille_BB.m:123`.
    56	  The 3D specifics (D3Q19 velocity set, ω_i weights) are NOT in this
    57	  anchor — see `tools/integrity/docs/algebraic/d3q19.md` and
    58	  registry entry `[Algebraic_D3Q19]` (Phase 12 setup-2).
    59	- **Algebraic_D3Q19:** Pure derivation; no vendored upstream. The
    60	  derivation document at `tools/integrity/docs/algebraic/d3q19.md`
    61	  pins the 19 velocity vectors, the 3 weight values, the canonical
    62	  GPU-Sims direction ordering, the opposite-direction involution, and
    63	  canonical test points for the BGK equilibrium evaluator. The
    64	  per-test-point expected feq values were generated by
    65	  `tools/integrity/integrity/cat3_numerical/checks/d3q19_verify.py`
    66	  at setup-2 land time and checked into the repo at
    67	  `d3q19_equilibrium.expected.json`. Regeneration is a deliberate human
    68	  action if the derivation itself changes.
    69	
    70	## Not yet registered (intentional)
    71	
    72	- **Chakazul/Lenia (LeniaNDK):** Cited in `continuous-ca/lenia-fft/python/lenia_fft/presets.py:11`
    73	  but not vendored. Per the Layer 3 lenia audit
    74	  (`sims_lenia_chakazul_2026-05-14_architect3b.md`) the upstream master
    75	  resolves cleanly but the citation is master-HEAD-only (no historical
    76	  pin). Adding LeniaNDK to this registry requires vendoring it under
    77	  `references/Chakazul-Lenia/` at a chosen anchor SHA. Deferred — this
    78	  is the test case for `cat1.unregistered-upstream`.
```

### E.3 — Registry suitability for cat1.bare-path upstream-index

INFERENCE (E.3): the registry's `vendor_root` field is *sufficient* for
cat1.bare-path to build the upstream-side basename index — every
registered entry that has a `vendor_root` (SPlisHSPlasH, Krueger) gives
the check a directory to recursively basename-index. The
`Algebraic_D3Q19` entry has no `vendor_root` (E.2 line 40), which is
fine: it has no files to index.

INFERENCE: no registry-shape extension is required for A.3. The check
treats the absence of `vendor_root` as "this anchor contributes nothing
to the upstream basename index" and moves on. If a future v1.3 added a
per-anchor "expose-as-bare" toggle to bound the index further, that
would be additive and out-of-scope for A.3.

INFERENCE (caveat): `anchor_version` is *not consumed* by cat1.bare-path
— a bare citation has no version token, so the check cannot anchor-match
the way `upstream.py` does. The implicit model is: "if a basename
resolves uniquely under the union of vendor_roots, the cited content is
whatever lives at the current pinned anchor SHA." Phase-11.5 / Phase-12
discipline (snapshot the vendor at a pinned SHA, write the citation
once) makes this defensible. If the vendor tree is later re-anchored to
a different SHA, the cited line content can shift — but that is true of
the existing cat1.intra-repo check on dotted paths too, so no new
guarantee is at stake.

---

## § F — AMBIGUOUS sample (the disambiguation-list data)

### F.1 — Probe output verbatim

FACT (run of `python3 /tmp/probe_a3.py` at probe-end SHA):

```text
=== Tally ===
REGISTERED-UPSTREAM-BARE: 169
INTRA-REPO-BARE: 298
AMBIGUOUS: 246
UNRESOLVABLE: 250

=== AMBIGUOUS sample (15) ===

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/phase12_lattice_boltzmann.md:1153|main.cpp:1100
    -> continuous-ca/reaction-diffusion-3d/src/main.cpp
    -> particle-fluids/sph-water/src/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/Tests/main.cpp
    -> build-test-alembic/_deps/alembic-src/maya/AbcImport/main.cpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/phase12_lattice_boltzmann.md:1276|main.cpp:1168
    -> continuous-ca/reaction-diffusion-3d/src/main.cpp
    -> particle-fluids/sph-water/src/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/Tests/main.cpp
    -> build-test-alembic/_deps/alembic-src/maya/AbcImport/main.cpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/retro/integrity-toolkit-v1.1-batch1-addendum.md:144|main.cpp:1168
    -> continuous-ca/reaction-diffusion-3d/src/main.cpp
    -> particle-fluids/sph-water/src/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/Tests/main.cpp
    -> build-test-alembic/_deps/alembic-src/maya/AbcImport/main.cpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/retro/integrity-toolkit-v1.1-batch1-addendum.md:192|runner.py:133
    -> common/common-py/.venv/lib/python3.12/site-packages/_pytest/runner.py
    -> tools/integrity/integrity/runner.py

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/retro/integrity-toolkit-v1.1-batch1-addendum.md:203|runner.py:143
    -> common/common-py/.venv/lib/python3.12/site-packages/_pytest/runner.py
    -> tools/integrity/integrity/runner.py

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/retro/integrity-toolkit-v1.1-batch1-addendum.md:203|runner.py:144
    -> common/common-py/.venv/lib/python3.12/site-packages/_pytest/runner.py
    -> tools/integrity/integrity/runner.py

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/retro/integrity-toolkit-v1.1-batch1-addendum.md:293|runner.py:143
    -> common/common-py/.venv/lib/python3.12/site-packages/_pytest/runner.py
    -> tools/integrity/integrity/runner.py

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/retro/integrity-toolkit-v1.1-batch1.md:180|example.cpp:42
    -> build-test-alembic/_deps/nlohmann_json-src/docs/mkdocs/docs/integration/example.cpp
    -> build-test-alembic/_deps/nlohmann_json-src/docs/mkdocs/docs/integration/conan/example.cpp
    -> build-test-alembic/_deps/nlohmann_json-src/docs/mkdocs/docs/integration/vcpkg/example.cpp
    -> build-test-alembic/_deps/spdlog-src/example/example.cpp
    -> build-test-alembic/_deps/shaderc-src/third_party/spirv-headers/tests/example.cpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/retro/integrity-toolkit-v1.1-batch1.md:335|main.cpp:1168
    -> continuous-ca/reaction-diffusion-3d/src/main.cpp
    -> particle-fluids/sph-water/src/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/Tests/main.cpp
    -> build-test-alembic/_deps/alembic-src/maya/AbcImport/main.cpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/sims_prioritization_2026-05-14_triage.md:103|main.cpp:1789
    -> continuous-ca/reaction-diffusion-3d/src/main.cpp
    -> particle-fluids/sph-water/src/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/Tests/main.cpp
    -> build-test-alembic/_deps/alembic-src/maya/AbcImport/main.cpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/phase11_5_commit1_landing_2026-05-14.md:23|main.cpp:1349
    -> continuous-ca/reaction-diffusion-3d/src/main.cpp
    -> particle-fluids/sph-water/src/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/Tests/main.cpp
    -> build-test-alembic/_deps/alembic-src/maya/AbcImport/main.cpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/phase11_5_commit2a_landing_2026-05-14.md:332|main.cpp:666
    -> continuous-ca/reaction-diffusion-3d/src/main.cpp
    -> particle-fluids/sph-water/src/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/Tests/main.cpp
    -> build-test-alembic/_deps/alembic-src/maya/AbcImport/main.cpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/phase11_5_resume_probe_2026-05-15_architect1.md:1296|main.cpp:1284
    -> continuous-ca/reaction-diffusion-3d/src/main.cpp
    -> particle-fluids/sph-water/src/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/Tests/main.cpp
    -> build-test-alembic/_deps/alembic-src/maya/AbcImport/main.cpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/commoncpp_unexercised_2026-05-14_architect2.md:70|main.cpp:1214
    -> continuous-ca/reaction-diffusion-3d/src/main.cpp
    -> particle-fluids/sph-water/src/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/Tests/main.cpp
    -> build-test-alembic/_deps/alembic-src/maya/AbcImport/main.cpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/commoncpp_unexercised_2026-05-14_architect2.md:72|main.cpp:1215
    -> continuous-ca/reaction-diffusion-3d/src/main.cpp
    -> particle-fluids/sph-water/src/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/main.cpp
    -> build-test-alembic/_deps/alembic-src/python/PyAlembic/Tests/main.cpp
    -> build-test-alembic/_deps/alembic-src/maya/AbcImport/main.cpp

=== REGISTERED-UPSTREAM-BARE sample (8) ===

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/integrity-toolkit-spec.md:612|SPHKernels.h:14
    -> references/SPlisHSPlasH/SPlisHSPlasH/SPHKernels.h

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/phase11_5_commit2_verification_2026-05-14.md:93|TimeStepDFSPH.cpp:1306
    -> references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/phase11_5_commit2_verification_2026-05-14.md:107|TimeStepDFSPH.cpp:1383
    -> references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/phase11_5_probe3_2026-05-14_architect1.md:8|TimeStepDFSPH.cpp:735
    -> references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/phase11_5_probe3_2026-05-14_architect1.md:305|TimeStepDFSPH.cpp:324
    -> references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/phase11_5_probe3_2026-05-14_architect1.md:308|TimeStepDFSPH.h:29
    -> references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.h

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/phase11_5_probe3_2026-05-14_architect1.md:308|TimeStepDFSPH.cpp:34
    -> references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/phase11_5_probe3_2026-05-14_architect1.md:308|TimeStepDFSPH.cpp:311
    -> references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp

=== INTRA-REPO-BARE sample (8) ===

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/retro/integrity-toolkit-v1.1-batch1-addendum.md:107|snapshot.py:178
    -> tools/integrity/integrity/snapshot.py

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/integrity_v1_1_batch1_spec_2026-05-15_architect1.md:97|alembic_writer.hpp:11
    -> common/common-cpp/include/gpusims/alembic_writer.hpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/integrity_v1_1_batch1_spec_2026-05-15_architect1.md:97|vdb_writer.hpp:12
    -> common/common-cpp/include/gpusims/vdb_writer.hpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/commoncpp_consumers_2026-05-14_architect2.md:231|vdb_writer.hpp:33
    -> common/common-cpp/include/gpusims/vdb_writer.hpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/commoncpp_consumers_2026-05-14_architect2.md:233|vdb_writer.hpp:41
    -> common/common-cpp/include/gpusims/vdb_writer.hpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/integrity_build_6_landing_2026-05-14.md:110|vdb_writer.hpp:33
    -> common/common-cpp/include/gpusims/vdb_writer.hpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/integrity_build_6_landing_2026-05-14.md:112|alembic_writer.hpp:24
    -> common/common-cpp/include/gpusims/alembic_writer.hpp

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
docs/diagnostics/_audits/commoncpp_unexercised_2026-05-14_architect2.md:30|alembic_writer.hpp:24
    -> common/common-cpp/include/gpusims/alembic_writer.hpp

=== UNRESOLVABLE sample (10) ===
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  docs/integrity-toolkit-spec.md:441|nTimeStepDFSPH.cpp:1370
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  docs/integrity-toolkit-spec.md:443|file.cpp:42
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  docs/integrity-toolkit-spec.md:488|LeniaNDK.py:329
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  docs/integrity-toolkit-spec.md:853|LeniaNDK.py:329
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  docs/sim-specs/lenia-fft.md:62|LeniaNDK.py:329
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  docs/sim-specs/lenia-fft.md:93|LeniaNDK.py:329
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  docs/retro/integrity-toolkit-v1.1-batch1-addendum.md:141|comp.glsl:7
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  docs/retro/integrity-toolkit-v1.md:132|LeniaNDK.py:329
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  docs/diagnostics/_audits/phase11_5_commit2_verification_2026-05-14.md:17|comp.glsl:73
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  docs/diagnostics/_audits/phase12_lbm_probe_2026-05-15_architect1.md:1001|frag.glsl:1
```

### F.2 — Drift against self-review probe § G.2

FACT (counts from the v1.1 self-review probe at
`docs/diagnostics/_audits/integrity_v1_1_self_review_probe_2026-05-15_architect1.md:2207-2210`):

| Class | Self-review (prior) | Current probe | Δ |
|-------|---------------------|---------------|---|
| REGISTERED-UPSTREAM-BARE | 164 | 169 | +5 |
| INTRA-REPO-BARE | 290 | 298 | +8 |
| AMBIGUOUS | 226 | 246 | +20 |
| UNRESOLVABLE | 244 | 250 | +6 |
| **Total** | **924** | **963** | **+39** |

FACT: drift is positive in all four buckets; total grew 4.2%. The three
audit files added in this probe-cycle (`phase11_5_boundary_placement_probe…`,
`phase11_5_resume_probe…`, `phase12_lbm_…_probe…`, `phase12_lbm_predraft_probe…`,
the v1.1 self-review probe itself) and the in-flight Phase-12 commits
(`cdad2e2`, `c1a257d`) are the most likely drift sources — each new
audit doc carries dozens of bare-basename citations. INFERENCE: drift
rate of ~10-per-day at the v1.2 cadence is the working assumption for
A.3 sizing of "candidate findings at run time."

INFERENCE (F.2 / D.5 cross-check): the AMBIGUOUS bucket is **inflated**
by `build-test-alembic/_deps/**` (sample shows ≥4 of every 5 `main.cpp`
ambiguity candidates living under alembic deps). Those files are not
git-tracked, so they will not be in the cat1.bare-path basename index
at runtime; the corrected AMBIGUOUS count under `list_tracked_files`
discipline is likely closer to ~50-80, not 246. **A.3 spec section
should specify that the index is built from `list_tracked_files`, not
from `rglob('*')`, to match this corrected estimate.**

---

## § G — Classifier interface

### G.1 — Verbatim dump of `classify()` (grandfather.py)

FACT: located at grandfather.py:38-125. Total file LOC = 330.

```python
    38	def classify(finding: Finding) -> Classification:
    39	    """Classify a finding into a grandfather category. First match wins."""
    40	    f = finding.file
    41	    msg = finding.message
    42	    cid = finding.check_id
    43	
    44	    if cid == "cat2.public-symbol-used":
    45	        return Classification(
    46	            category="cat2-stack-d-unused",
    47	            reason="pre-v1 Stack D public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-d-unused)",
    48	            issue_ref="n/a",
    49	        )
    50	
    51	    if cid == "cat2.public-symbol-used-c":
    52	        return Classification(
    53	            category="cat2-stack-c-unused",
    54	            reason="pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused)",
    55	            issue_ref="n/a",
    56	        )
    57	
    58	    if cid == "cat2.public-symbol-used-ts":
    59	        return Classification(
    60	            category="cat2-stack-b-unused",
    61	            reason="pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused)",
    62	            issue_ref="n/a",
    63	        )
    64	
    65	    if cid == "cat2.stub-label-stale":
    66	        return Classification(
    67	            category="cat2-stub-label-stale",
    68	            reason="pre-v1.1 stale Phase-N stub label on real implementation (canonical spec section 12 row 5 -- tracked for migration as the corresponding header is next edited)",
    69	            issue_ref="n/a",
    70	        )
    71	
    72	    if cid == "cat1.intra-repo" and f.startswith("docs/diagnostics/_audits/"):
    73	        return Classification(
    74	            category="audit-citation",
    75	            reason="audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation)",
    76	            issue_ref="n/a",
    77	        )
    78	
    79	    if cid == "cat1.upstream-citation" and "1.8.10" in msg:
    80	        if (
    81	            f.startswith("particle-fluids/sph-water/shaders/")
    82	            or f.startswith("particle-fluids/sph-water/src/")
    83	        ):
    84	            return Classification(
    85	                category="live-shader-1810",
    86	                reason="pre-v1 SPlisHSPlasH 1.8.10 anchor in live code (migration target tracked in grandfather-catalog live-shader-1810)",
    87	                issue_ref="n/a",
    88	            )
    89	        return Classification(
    90	            category="audit-doc-1810",
    91	            reason="audit-doc reference to the historical 1.8.10 fabrication (permanent suppression)",
    92	            issue_ref="n/a",
    93	        )
    94	
    95	    if cid == "cat1.annotation-form":
    96	        if f == "docs/integrity-toolkit-spec.md" or f.startswith("tools/integrity/docs/"):
    97	            return Classification(
    98	                category="spec-grammar-example",
    99	                reason="documentation-only literal mention of the annotation grammar (not a real annotation)",
    100	                issue_ref="n/a",
    101	            )
    102	        if f.startswith("docs/retro/"):
    103	            return Classification(
    104	                category="retro-grammar-example",
    105	                reason="retrospective-doc literal mention of the annotation grammar (not a real annotation)",
    106	                issue_ref="n/a",
    107	            )
    108	        if f.startswith("tools/integrity/integrity/"):
    109	            return Classification(
    110	                category="toolkit-own-source",
    111	                reason="regex or docstring literal of the annotation grammar token (not a real annotation)",
    112	                issue_ref="n/a",
    113	            )
    114	        if f.startswith("docs/diagnostics/_audits/"):
    115	            return Classification(
    116	                category="audit-report-grammar-example",
    117	                reason="audit-doc literal mention of the annotation grammar (not a real annotation)",
    118	                issue_ref="n/a",
    119	            )
    120	
    121	    return Classification(
    122	        category="other-cat1",
    123	        reason="grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a",
    124	        issue_ref="n/a",
    125	    )
```

### G.2 — Path-prefix patterns used by the existing classifier

FACT (extracted from G.1):

| Prefix pattern | Routing target (category) | Source line |
|----------------|--------------------------|-------------|
| `docs/diagnostics/_audits/` (cat1.intra-repo) | `audit-citation` | grandfather.py:72 |
| `particle-fluids/sph-water/shaders/` (cat1.upstream + "1.8.10") | `live-shader-1810` | grandfather.py:81 |
| `particle-fluids/sph-water/src/` (cat1.upstream + "1.8.10") | `live-shader-1810` | grandfather.py:82 |
| `docs/integrity-toolkit-spec.md` exact (cat1.annotation-form) | `spec-grammar-example` | grandfather.py:96 |
| `tools/integrity/docs/` (cat1.annotation-form) | `spec-grammar-example` | grandfather.py:96 |
| `docs/retro/` (cat1.annotation-form) | `retro-grammar-example` | grandfather.py:102 |
| `tools/integrity/integrity/` (cat1.annotation-form) | `toolkit-own-source` | grandfather.py:108 |
| `docs/diagnostics/_audits/` (cat1.annotation-form) | `audit-report-grammar-example` | grandfather.py:114 |
| (default fall-through) | `other-cat1` | grandfather.py:121 |

INFERENCE (G.2): the **new cat1.bare-path classifier rules** will mirror
the cat1.intra-repo `docs/diagnostics/_audits/` rule first (most audit
bare-path citations land there). A reasonable rule set, in match-order:

```python
    if cid == "cat1.bare-path" and f.startswith("docs/diagnostics/_audits/"):
        return Classification(category="audit-bare-path",
                              reason="audit-doc snapshot pre-v1.2 (see grandfather-catalog audit-bare-path)",
                              issue_ref="n/a")
    if cid == "cat1.bare-path" and f.startswith("docs/retro/"):
        return Classification(category="retro-bare-path", ...)
    if cid == "cat1.bare-path" and f == "docs/integrity-toolkit-spec.md":
        return Classification(category="spec-bare-path-example", ...)
    if cid == "cat1.bare-path":
        return Classification(category="other-cat1-bare-path", ...)
```

INFERENCE: these classifier additions are pure-append before the
final default `return` at grandfather.py:121, so the change is
diff-minimal and the prior categories continue to work unchanged.

### G.3 — `Classification` dataclass shape

FACT (grandfather.py:31-36):

```python
    31	@dataclass(frozen=True)
    32	class Classification:
    33	    category: str
    34	    reason: str
    35	    issue_ref: str
```

FACT confirmation: shape is exactly `{category: str, reason: str,
issue_ref: str}`. INFERENCE: this matches the v1.1 apispec probe's
description. No drift since v1.1; A.3's classifier rules use the same
constructor with no schema changes.

---

## § H — common/repo.py

### H.1 — Verbatim dump

FACT: total LOC = 41.

```python
     1	"""Git / repo helpers."""
     2	
     3	from __future__ import annotations
     4	
     5	import subprocess
     6	from pathlib import Path
     7	
     8	
     9	def find_repo_root(start: Path | None = None) -> Path:
    10	    """Walk upward from `start` (default cwd) to find the git repo root."""
    11	    cwd = start if start else Path.cwd()
    12	    cur = cwd.resolve()
    13	    while cur != cur.parent:
    14	        if (cur / ".git").exists():
    15	            return cur
    16	        cur = cur.parent
    17	    raise RuntimeError(f"No git repo found from {cwd}")
    18	
    19	
    20	def git_head_sha(root: Path) -> str:
    21	    """Return short HEAD SHA for the repo at `root`."""
    22	    result = subprocess.run(
    23	        ["git", "rev-parse", "--short", "HEAD"],
    24	        cwd=root,
    25	        capture_output=True,
    26	        text=True,
    27	        check=True,
    28	    )
    29	    return result.stdout.strip()
    30	
    31	
    32	def list_tracked_files(root: Path) -> list[Path]:
    33	    """Return all git-tracked files under `root` as repo-relative paths."""
    34	    result = subprocess.run(
    35	        ["git", "ls-files"],
    36	        cwd=root,
    37	        capture_output=True,
    38	        text=True,
    39	        check=True,
    40	    )
    41	    return [root / line for line in result.stdout.splitlines() if line]
```

### H.2 — `list_tracked_files` return type

FACT (H.2): `list_tracked_files(repo_root)` returns `list[Path]` where
each element is constructed as `root / line` (repo.py:41). `root` is the
function argument; if the caller passes an absolute Path (as
`intra_repo.py` and `upstream.py` both do via `repo_root` argument),
each element is an absolute `Path`. The docstring says "as
repo-relative paths" — that is **misleading**: the returned values are
`root / <relative-path-string>`, which is absolute when `root` is
absolute.

INFERENCE: the new cat1.bare-path check is fine to consume
`list_tracked_files` directly, the same way `intra_repo.py:74` already
does (it then immediately calls `absolute.relative_to(repo_root)` at
line 75 to make a relative `str` for `is_excluded`). The bare-path check
will follow the same idiom: iterate the absolute Paths, build the
upstream-side index (filtered to those under `references/<vendor_root>`)
and the intra-repo-side index (filtered to those *not* under
`references/`), then walk the same list a second time to scan source
files for bare-path citations.

INFERENCE: the index-build pass is single-pass O(n) over the same
git-tracked file list the check already iterates. No additional
subprocess shells, no extra `find` walks.

---

## Closing notes

**Read-only?** FACT: yes. Probe-start SHA equals probe-end SHA:
`9add1494b237e33f3dda782c821b9d7f29446068`.

**Open questions surfaced by the probe (for A.3 spec consumption, not
resolved here):**

1. AMBIGUOUS policy (HARD_FAIL vs WARN-with-disambiguation-suggestion).
   Existing grandfather-catalog idiom is HARD_FAIL + classifier-routed
   suppression; AMBIGUOUS likely follows that pattern but the *suggestion
   payload* (list of candidate paths) wants spec attention.
2. UNRESOLVABLE policy. The 250-bucket includes both author errors (typos)
   and known-deferred citations (`LeniaNDK.py`, the unregistered Chakazul
   upstream — see E.2 lines 72-78). The classifier needs a rule for the
   deferred-upstream case so it doesn't get lumped into `other-cat1-bare-path`.
3. Index-scope policy. INFERENCE (D.5 / F.2 cross-check) is that
   `list_tracked_files` discipline is the right scope. A.3 spec should
   say so explicitly so the check doesn't accidentally re-include
   `build-test-alembic/_deps/**` if the file iterator is ever reworked.
4. The cat1.intra-repo skip-guard (B.2) needs a corresponding test in
   `tests/test_cat1_intra_repo.py` — bare-basename citations are
   currently part of cat1.intra-repo's HARD_FAIL surface and the
   transition to cat1.bare-path will move those expected findings to
   the new check. A.3 spec needs to call out the test-migration step.
