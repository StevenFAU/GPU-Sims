"""Stack C public-API surface extractor + reference finder per spec § 7.2, § 7.3.

The "public" surface for Stack C is whatever is declared in headers under
common/common-cpp/include/gpusims/ (recursively).

Uses libclang via the `clang.cindex` Python bindings. Requires
compile_commands.json to be available at build/compile_commands.json
(produced by `cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`).

USR-based reference matching gives type-aware semantics; name collisions
with unrelated classes do not confuse the check.
"""

from __future__ import annotations

import json
from dataclasses import dataclass
from enum import Enum
from pathlib import Path

try:
    import clang.cindex  # type: ignore[import-not-found]
except ImportError as e:
    raise RuntimeError(
        "libclang Python bindings unavailable. Install via "
        "`pip install libclang` and ensure libclang.so is on the system."
    ) from e


COMMON_CPP_PUBLIC_DIR = Path("common/common-cpp/include/gpusims")
COMMON_CPP_IMPL_DIR = Path("common/common-cpp/src")
BUILD_COMPILE_COMMANDS = Path("build/compile_commands.json")


class SymbolKind(Enum):
    CLASS = "class"
    STRUCT = "struct"
    CLASS_FIELD = "class_field"
    FREE_FUNCTION = "free_function"
    METHOD = "method"


@dataclass(frozen=True)
class PublicSymbol:
    name: str
    qualified_name: str
    usr: str
    kind: SymbolKind
    defining_file: Path
    defining_line: int
    parent_class: str | None
    parent_class_usr: str | None


def _load_compile_args(repo_root: Path, source_file: Path) -> list[str]:
    """Return the compile args for `source_file` from compile_commands.json,
    or a sensible default."""
    db_path = repo_root / BUILD_COMPILE_COMMANDS
    default = ["-std=c++20"]
    candidate_includes = [
        repo_root / "common" / "common-cpp" / "include",
        repo_root / "common" / "common-cpp" / "src",
        repo_root / "include",
    ]
    for inc in candidate_includes:
        if inc.is_dir():
            default.extend(["-I", str(inc)])

    if not db_path.is_file():
        return default

    try:
        entries = json.loads(db_path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError):
        return default

    try:
        source_resolved = source_file.resolve()
    except OSError:
        return default

    for entry in entries:
        entry_file = entry.get("file", "")
        try:
            if Path(entry_file).resolve() != source_resolved:
                continue
        except OSError:
            continue
        cmd = entry.get("command") or " ".join(entry.get("arguments", []))
        tokens = cmd.split()
        args: list[str] = []
        skip_next = False
        for i, tok in enumerate(tokens):
            if i == 0:
                continue
            if skip_next:
                skip_next = False
                continue
            if tok == "-o":
                skip_next = True
                continue
            if tok == "-c":
                continue
            if tok == str(source_resolved) or tok.endswith("/" + source_file.name):
                continue
            args.append(tok)
        return args

    return default


def _find_representative_tu(repo_root: Path) -> Path | None:
    """Find a .cpp file that transitively includes most public headers."""
    hello_main = repo_root / "common" / "common-cpp" / "examples" / "hello" / "main.cpp"
    if hello_main.is_file():
        return hello_main
    src_dir = repo_root / COMMON_CPP_IMPL_DIR
    if src_dir.is_dir():
        for p in sorted(src_dir.rglob("*.cpp")):
            return p
    # Fallback: any .cpp under repo_root/src (covers test fixtures)
    alt_src = repo_root / "src"
    if alt_src.is_dir():
        for p in sorted(alt_src.rglob("*.cpp")):
            return p
    return None


def extract_public_surface(repo_root: Path) -> list[PublicSymbol]:
    """Walk public headers via representative TUs and extract every public declaration.

    Uses multiple .cpp files from common-cpp/src/ (plus the hello example
    if present) so that the union of #includes covers every public
    header. Dedupes symbols by USR across TUs.
    """
    public_dir = (repo_root / COMMON_CPP_PUBLIC_DIR).resolve()
    if not public_dir.is_dir():
        alt = (repo_root / "include").resolve()
        if alt.is_dir():
            public_dir = alt
        else:
            return []

    tu_sources = _representative_tus(repo_root)
    if not tu_sources:
        return []

    symbols: list[PublicSymbol] = []
    seen_usrs: set[str] = set()
    index = clang.cindex.Index.create()
    for tu_source in tu_sources:
        args = _load_compile_args(repo_root, tu_source)
        try:
            tu = index.parse(
                str(tu_source), args=args,
                options=clang.cindex.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD,
            )
        except clang.cindex.TranslationUnitLoadError:
            continue
        _walk_for_public_decls(tu.cursor, public_dir, symbols, seen_usrs, class_stack=[])

    # Cross-TU dedup: same (file, line, name, kind) can yield distinct
    # USRs when a header is parsed under multiple namespace contexts.
    # Keep one symbol per logical location.
    deduped: list[PublicSymbol] = []
    seen_locs: set[tuple[str, int, str, str]] = set()
    for s in symbols:
        key = (str(s.defining_file), s.defining_line, s.name, s.kind.value)
        if key in seen_locs:
            continue
        seen_locs.add(key)
        deduped.append(s)
    return deduped


def _representative_tus(repo_root: Path) -> list[Path]:
    """Return a set of .cpp files whose union of #includes covers the
    public header surface."""
    tus: list[Path] = []
    hello_main = repo_root / "common" / "common-cpp" / "examples" / "hello" / "main.cpp"
    if hello_main.is_file():
        tus.append(hello_main)
    src_dir = repo_root / COMMON_CPP_IMPL_DIR
    if src_dir.is_dir():
        tus.extend(sorted(src_dir.rglob("*.cpp")))
    alt_src = repo_root / "src"
    if alt_src.is_dir() and not src_dir.is_dir():
        tus.extend(sorted(alt_src.rglob("*.cpp")))
    return tus


def _walk_for_public_decls(
    cursor,
    public_dir: Path,
    symbols: list[PublicSymbol],
    seen_usrs: set[str],
    class_stack: list[tuple[str, str]],
) -> None:
    """Recursive cursor walk. Collects public declarations whose location
    is in a public header."""
    CursorKind = clang.cindex.CursorKind
    AccessSpecifier = clang.cindex.AccessSpecifier

    # Translation unit root: recurse without location filtering.
    if cursor.kind == CursorKind.TRANSLATION_UNIT:
        for child in cursor.get_children():
            _walk_for_public_decls(child, public_dir, symbols, seen_usrs, class_stack)
        return

    if cursor.kind == CursorKind.NAMESPACE:
        for child in cursor.get_children():
            _walk_for_public_decls(child, public_dir, symbols, seen_usrs, class_stack)
        return

    if cursor.location.file is None:
        return

    try:
        cur_file = Path(cursor.location.file.name).resolve()
    except OSError:
        return

    try:
        cur_file.relative_to(public_dir)
        in_public_header = True
    except ValueError:
        in_public_header = False

    if not in_public_header:
        return

    usr = cursor.get_usr() or ""

    if cursor.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL):
        if cursor.is_definition() and usr and usr not in seen_usrs:
            kind = SymbolKind.CLASS if cursor.kind == CursorKind.CLASS_DECL else SymbolKind.STRUCT
            symbols.append(PublicSymbol(
                name=cursor.spelling,
                qualified_name=_qualified_name(cursor),
                usr=usr,
                kind=kind,
                defining_file=cur_file,
                defining_line=cursor.location.line,
                parent_class=class_stack[-1][0] if class_stack else None,
                parent_class_usr=class_stack[-1][1] if class_stack else None,
            ))
            seen_usrs.add(usr)
        if cursor.is_definition():
            new_stack = class_stack + [(cursor.spelling, usr)] if usr else class_stack
            for child in cursor.get_children():
                _walk_for_public_decls(child, public_dir, symbols, seen_usrs, new_stack)
        return

    if cursor.kind == CursorKind.FIELD_DECL:
        if cursor.access_specifier in (AccessSpecifier.PRIVATE, AccessSpecifier.PROTECTED):
            return
        if not class_stack or not usr or usr in seen_usrs:
            return
        if cursor.spelling.startswith("_"):
            return
        symbols.append(PublicSymbol(
            name=cursor.spelling,
            qualified_name=_qualified_name(cursor),
            usr=usr,
            kind=SymbolKind.CLASS_FIELD,
            defining_file=cur_file,
            defining_line=cursor.location.line,
            parent_class=class_stack[-1][0],
            parent_class_usr=class_stack[-1][1],
        ))
        seen_usrs.add(usr)
        return

    if cursor.kind == CursorKind.CXX_METHOD:
        if cursor.access_specifier in (AccessSpecifier.PRIVATE, AccessSpecifier.PROTECTED):
            return
        if not class_stack or not usr or usr in seen_usrs:
            return
        # Skip constructors / destructors / operator overloads — they're
        # used implicitly and reporting them generates noise.
        if cursor.spelling.startswith("operator") or cursor.spelling.startswith("~"):
            return
        symbols.append(PublicSymbol(
            name=cursor.spelling,
            qualified_name=_qualified_name(cursor),
            usr=usr,
            kind=SymbolKind.METHOD,
            defining_file=cur_file,
            defining_line=cursor.location.line,
            parent_class=class_stack[-1][0],
            parent_class_usr=class_stack[-1][1],
        ))
        seen_usrs.add(usr)
        return

    if cursor.kind == CursorKind.FUNCTION_DECL:
        if not usr or usr in seen_usrs:
            return
        symbols.append(PublicSymbol(
            name=cursor.spelling,
            qualified_name=_qualified_name(cursor),
            usr=usr,
            kind=SymbolKind.FREE_FUNCTION,
            defining_file=cur_file,
            defining_line=cursor.location.line,
            parent_class=None,
            parent_class_usr=None,
        ))
        seen_usrs.add(usr)
        return

    for child in cursor.get_children():
        _walk_for_public_decls(child, public_dir, symbols, seen_usrs, class_stack)


def _qualified_name(cursor) -> str:
    """Build a fully-qualified name like 'gpusims::abc::ParticleFrame::radii'."""
    parts: list[str] = []
    cur = cursor
    while cur is not None and cur.kind != clang.cindex.CursorKind.TRANSLATION_UNIT:
        if cur.spelling:
            parts.append(cur.spelling)
        cur = cur.semantic_parent
    return "::".join(reversed(parts))


def find_references(
    repo_root: Path,
    symbols: list[PublicSymbol],
    consumer_sources: list[Path],
) -> dict[str, list[tuple[Path, int]]]:
    """For each symbol's USR, find references in consumer_sources.

    Returns a dict mapping USR → list of (file, line). Non-self filter
    applied for CLASS_FIELD and METHOD symbols.

    Two passes per source: (1) cursor walk via referenced.get_usr() for
    classes / functions / methods / fields that libclang exposes
    directly; (2) token scan for class-field accesses that libclang
    hides inside UNEXPOSED_EXPR (chained member access like
    `f.positions.size()` — the intermediate member-ref isn't exposed
    as a named cursor in the AST walk).
    """
    target_usrs = {s.usr: s for s in symbols}
    # Build a per-name index of FIELD targets for the token-scan pass.
    field_targets_by_name: dict[str, list[PublicSymbol]] = {}
    for s in symbols:
        if s.kind == SymbolKind.CLASS_FIELD:
            field_targets_by_name.setdefault(s.name, []).append(s)

    refs: dict[str, list[tuple[Path, int]]] = {usr: [] for usr in target_usrs}

    index = clang.cindex.Index.create()
    for source in consumer_sources:
        if not source.is_file():
            continue
        args = _load_compile_args(repo_root, source)
        try:
            tu = index.parse(str(source), args=args)
        except clang.cindex.TranslationUnitLoadError:
            continue

        _collect_refs(tu.cursor, target_usrs, refs, class_stack=[])
        if field_targets_by_name:
            _collect_field_token_refs(
                tu, source, field_targets_by_name, refs,
            )

    return refs


def _find_matching_field_at_token(
    cursor,
    line: int,
    candidates: list[PublicSymbol],
) -> PublicSymbol | None:
    """Walk into `cursor` looking for a sub-cursor at `line` whose
    `referenced.get_usr()` matches one of the candidate field USRs."""
    # Check this cursor first
    ref = cursor.referenced
    if ref is not None:
        ref_usr = ref.get_usr()
        if ref_usr:
            target = next((c for c in candidates if c.usr == ref_usr), None)
            if target is not None:
                return target
    # Then walk children that overlap this line
    for child in cursor.get_children():
        if child.location.file is None:
            continue
        child_line = child.location.line
        # Constrain the recursion to the relevant region
        try:
            extent = child.extent
            start_line = extent.start.line
            end_line = extent.end.line
        except Exception:
            start_line = end_line = child_line
        if start_line <= line <= end_line:
            result = _find_matching_field_at_token(child, line, candidates)
            if result is not None:
                return result
    return None


def _collect_field_token_refs(
    tu,
    source: Path,
    field_targets_by_name: dict[str, list[PublicSymbol]],
    refs: dict[str, list[tuple[Path, int]]],
) -> None:
    """Token-scan pass: walks every token in the TU; for each IDENTIFIER
    token matching a known field name AND preceded by `.` or `->`, count
    as a member-access reference. Pragmatic fallback for libclang's
    UNEXPOSED_EXPR hiding of chained member access (e.g.
    `f.positions.size()` — the inner member-ref is not exposed in the
    AST walk).

    Since the underlying class context isn't AST-resolved here, we don't
    apply USR matching: any `.fieldname` token in any consumer file
    counts as a reference if `fieldname` matches a known target. This
    has the same "name collision evades detection" false-MISS class as
    Stack D's AST-attribute matching (acceptable per spec for v1)."""
    TokenKind = clang.cindex.TokenKind

    # Group tokens by (file, line) and walk pairwise so we can check
    # the preceding token's spelling.
    prev_token = None
    for token in tu.cursor.get_tokens():
        spelling = token.spelling
        candidates = field_targets_by_name.get(spelling)
        if (token.kind == TokenKind.IDENTIFIER and
                candidates is not None and
                prev_token is not None and
                prev_token.spelling in (".", "->")):
            loc = token.location
            if loc.file is not None:
                try:
                    tok_path = Path(loc.file.name).resolve()
                except OSError:
                    tok_path = None
                if tok_path is not None:
                    for target in candidates:
                        # Skip the decl site itself
                        if (tok_path == target.defining_file.resolve() and
                                loc.line == target.defining_line):
                            continue
                        sites = refs[target.usr]
                        ref_loc = (Path(loc.file.name), loc.line)
                        if ref_loc not in sites:
                            sites.append(ref_loc)
                        # Only attribute to first candidate (we can't
                        # disambiguate without type info; this matches
                        # the Stack D semantics)
                        break
        prev_token = token


def _collect_refs(
    cursor,
    target_usrs: dict[str, PublicSymbol],
    refs: dict[str, list[tuple[Path, int]]],
    class_stack: list[str],
) -> None:
    """Walk a TU and collect references to target USRs."""
    CursorKind = clang.cindex.CursorKind

    if cursor.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL):
        usr = cursor.get_usr() or ""
        new_stack = class_stack + [usr] if usr else class_stack
        for child in cursor.get_children():
            _collect_refs(child, target_usrs, refs, new_stack)
        return

    # Skip declarations/definitions of the target itself: they reference
    # themselves but they're not consumers.
    DECL_KINDS = {
        CursorKind.FUNCTION_DECL,
        CursorKind.CXX_METHOD,
        CursorKind.FIELD_DECL,
        CursorKind.CLASS_DECL,
        CursorKind.STRUCT_DECL,
        CursorKind.VAR_DECL,
        CursorKind.PARM_DECL,
    }
    if cursor.kind not in DECL_KINDS:
        referenced = cursor.referenced
        if referenced is not None:
            ref_usr = referenced.get_usr()
            if ref_usr in target_usrs:
                target = target_usrs[ref_usr]
                if cursor.location.file is not None:
                    skip = False
                    if target.kind in (SymbolKind.CLASS_FIELD, SymbolKind.METHOD):
                        if target.parent_class_usr and target.parent_class_usr in class_stack:
                            skip = True
                    if not skip:
                        refs[ref_usr].append(
                            (Path(cursor.location.file.name), cursor.location.line)
                        )

    for child in cursor.get_children():
        _collect_refs(child, target_usrs, refs, class_stack)


def _parse_translation_units(
    repo_root: Path,
    sources: list[Path],
) -> list[tuple[Path, "clang.cindex.TranslationUnit"]]:
    """Parse each TU exactly once. Returns (source, tu) pairs in input order.

    Skips sources that don't exist on disk and TUs that fail to parse
    (matches the silent-skip behavior of the prior extract / find paths).
    """
    index = clang.cindex.Index.create()
    parsed: list[tuple[Path, clang.cindex.TranslationUnit]] = []
    for source in sources:
        if not source.is_file():
            continue
        args = _load_compile_args(repo_root, source)
        try:
            tu = index.parse(
                str(source), args=args,
                options=clang.cindex.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD,
            )
        except clang.cindex.TranslationUnitLoadError:
            continue
        parsed.append((source, tu))
    return parsed


def extract_and_find_references(
    repo_root: Path,
) -> tuple[list[PublicSymbol], dict[str, list[tuple[Path, int]]]]:
    """Single-parse Stack C entry point (T2.3).

    Parses the union of representative TUs and consumer sources exactly
    once, then runs both the public-symbol extraction pass and the
    reference-finding pass against the cached parsed TUs. Eliminates the
    double-parse pattern of the legacy `extract_public_surface` +
    `find_references` flow, which re-instantiated `clang.cindex.Index`
    twice and reparsed every source that appears in both sets.

    Returns:
        (public_symbols, refs_by_usr)
        Semantics identical to the legacy two-call path: symbols are
        extracted only from representative TUs; refs are collected only
        from consumer sources. The single-parse change is transparent
        to callers.
    """
    public_dir = (repo_root / COMMON_CPP_PUBLIC_DIR).resolve()
    if not public_dir.is_dir():
        alt = (repo_root / "include").resolve()
        if alt.is_dir():
            public_dir = alt
        else:
            return [], {}

    tu_sources = _representative_tus(repo_root)
    consumer_sources = discover_consumer_sources(repo_root)
    if not tu_sources and not consumer_sources:
        return [], {}

    # Resolve & dedupe so the union parse touches each file once.
    def _resolve_set(paths: list[Path]) -> set[Path]:
        out: set[Path] = set()
        for p in paths:
            try:
                out.add(p.resolve())
            except OSError:
                continue
        return out

    tu_resolved = _resolve_set(tu_sources)
    consumer_resolved = _resolve_set(consumer_sources)
    union_sources = sorted(tu_resolved | consumer_resolved)

    parsed = _parse_translation_units(repo_root, union_sources)

    # Extraction pass: only on representative-TU sources.
    symbols: list[PublicSymbol] = []
    seen_usrs: set[str] = set()
    for source, tu in parsed:
        if source not in tu_resolved:
            continue
        _walk_for_public_decls(tu.cursor, public_dir, symbols, seen_usrs, class_stack=[])

    # Cross-TU dedup (same shape as extract_public_surface).
    deduped: list[PublicSymbol] = []
    seen_locs: set[tuple[str, int, str, str]] = set()
    for s in symbols:
        key = (str(s.defining_file), s.defining_line, s.name, s.kind.value)
        if key in seen_locs:
            continue
        seen_locs.add(key)
        deduped.append(s)

    if not deduped:
        return [], {}

    # Reference pass: only on consumer sources.
    target_usrs = {s.usr: s for s in deduped}
    field_targets_by_name: dict[str, list[PublicSymbol]] = {}
    for s in deduped:
        if s.kind == SymbolKind.CLASS_FIELD:
            field_targets_by_name.setdefault(s.name, []).append(s)
    refs: dict[str, list[tuple[Path, int]]] = {usr: [] for usr in target_usrs}

    for source, tu in parsed:
        if source not in consumer_resolved:
            continue
        _collect_refs(tu.cursor, target_usrs, refs, class_stack=[])
        if field_targets_by_name:
            _collect_field_token_refs(tu, source, field_targets_by_name, refs)

    return deduped, refs


# integrity-allow: cat2.public-symbol-used-toolkit; pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused); n/a
def discover_consumer_sources(repo_root: Path) -> list[Path]:
    """List C++ source files that should be scanned for consumers."""
    consumers: list[Path] = []
    impl_dir = repo_root / COMMON_CPP_IMPL_DIR
    if impl_dir.is_dir():
        consumers.extend(sorted(impl_dir.rglob("*.cpp")))
    # Test-fixture fallback: <repo_root>/src
    alt_src = repo_root / "src"
    if alt_src.is_dir() and not impl_dir.is_dir():
        consumers.extend(sorted(alt_src.rglob("*.cpp")))
    examples_dir = repo_root / "common" / "common-cpp" / "examples"
    if examples_dir.is_dir():
        consumers.extend(sorted(examples_dir.rglob("*.cpp")))
    sim_roots = [
        repo_root / "particle-fluids",
        repo_root / "volumetric-grid",
        repo_root / "continuous-ca",
        repo_root / "hybrid-particle-grid",
    ]
    for sim_root in sim_roots:
        if not sim_root.is_dir():
            continue
        for src_dir in sim_root.rglob("src"):
            if src_dir.is_dir():
                consumers.extend(sorted(src_dir.rglob("*.cpp")))
    return consumers
