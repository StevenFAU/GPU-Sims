# Contributing to GPU-Sims

This repo is a personal portfolio project, not a community project. External contributions are not actively solicited, but the repo is public and the design documentation is open for reference and learning. If you find a bug, have a question, or want to discuss a technique, GitHub issues are welcome.

## Project structure

- [`docs/overarching-spec.md`](docs/overarching-spec.md) — authoritative cross-cutting spec for the entire repo
- [`docs/root-context-distilled.md`](docs/root-context-distilled.md) — reasoning and rejected alternatives
- [`docs/conventions.md`](docs/conventions.md) — coding, shader, naming, and profiling conventions
- [`docs/sim-specs/`](docs/sim-specs/) — one specification sheet per simulation
- [`common/`](common/) — shared infrastructure consumed by every native, web, and Python sim
- Sim folders, organized by category (`volumetric-grid/`, `particle-fluids/`, etc.)

## How a new simulation is added

1. Open a `sim_proposal` issue (template provided) describing the proposed sim, its category, the stack it should target, and the scientific or visual goal.
2. A specification sheet is drafted at `docs/sim-specs/<sim-name>.md` using the [template](docs/sim-specs/_template.md).
3. The sim is implemented in its category folder, consuming utilities from `common/`.
4. The sim's README, performance numbers, and gallery assets are filled in as the implementation matures.

## Coding conventions

See [`docs/conventions.md`](docs/conventions.md). Key points:

- **C++20** with `clang-format` (project's `.clang-format`)
- **Python 3.11+** with `ruff`
- **TypeScript strict mode** with `prettier` and `eslint`
- **Shaders** live next to their host code and are hot-reloaded
- **Profiling** is integrated from day one; performance regressions are caught against documented baselines

## Commit messages

Conventional Commits format is encouraged but not enforced. Categories:

- `feat:` new feature or sim
- `fix:` bug fix
- `perf:` performance improvement
- `docs:` documentation change
- `refactor:` code change that is neither a fix nor a feature
- `chore:` build, CI, tooling
- `style:` formatting only

## License

By contributing, you agree your contribution is licensed under the MIT License (see [`LICENSE`](LICENSE)).
