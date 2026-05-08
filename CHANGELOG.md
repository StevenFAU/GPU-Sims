# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- (Phase 1) Shared infrastructure under `common/` (camera, hot-reload, profiling, state-capture, VDB/Alembic export).
- (Phase 1) Top-level CMake build system with hello-world native sim.
- (Phase 2+) Individual simulation implementations.

## [0.1.0] — 2026-05-08

### Added
- Initial repository skeleton (Phase 0).
- Authoritative specification documents: `docs/overarching-spec.md`, `docs/root-context-distilled.md`.
- Per-sim spec sheet template at `docs/sim-specs/_template.md`.
- Stub spec sheets for all 14 simulations in the catalog.
- Sim category and per-sim README stubs.
- `render-pipelines/` skeleton with Blender (default), Houdini, and OptiX subfolders.
- Repository hygiene: `LICENSE` (MIT), `.gitignore`, `.gitattributes`, `.editorconfig`, `.clang-format`.
- GitHub-surface files: `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md`, `CITATION.cff`, issue and PR templates.
- CI workflows for markdown linting and structure validation.

[Unreleased]: https://github.com/StevenFAU/GPU-Sims/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/StevenFAU/GPU-Sims/releases/tag/v0.1.0
