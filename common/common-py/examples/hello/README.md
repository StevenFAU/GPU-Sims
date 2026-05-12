# hello-py — common-py reference application

Minimal Stack D sim that exercises every `gpusims_common` module end-to-end.
Copy this directory as the starting template for a new Stack D sim.

## Run

```bash
cd common/common-py/examples/hello
python3 -m venv .venv && source .venv/bin/activate
pip install -e .
python main.py
```

`pip install -e .` resolves `gpusims-common-py` from `../../`. If you've not
installed `common-py` editable yet, run `pip install -e ../..` first.

## Controls

| Key | Action |
|---|---|
| `WASDQE` | Move (hold RMB to look around) |
| `F5` | Save state to `captures/capture_NNNN/` |
| `F9` | Load latest capture |
| `R` | Reset particles |
| `Space` | Pause / unpause |
| `Esc` | Quit |

## Exports

Toggle VDB / Alembic export from the "Export" panel. **VDB**: writes
`vdb_export/hello_density_NNNN.vdb` every 4 frames if `pyopenvdb` is importable
(install via `sudo apt install python3-openvdb` on Ubuntu), otherwise logs a
stub warning and skips. **Alembic**: always logs a stub warning in Phase 9 —
real Python Alembic support is banked for the sph-water phase per
`project-state.md` § 7 rule-of-three convention.

## Walking the code

`main.py` is intentionally short (~200 lines) and structured so each
`gpusims_common` import gets a clear exercise site:

| import | used at |
|---|---|
| `Camera`, `CameraMode` | construction + `track_user_inputs(window)` in the render loop |
| `ParamPanel` | `bind(gui)` + `folder(...)` context managers for sliders + buttons |
| `StateWriter` / `StateReader` | F5 / F9 handlers |
| `VdbWriter` | per-frame density-histogram export under the "VDB" checkbox |
| `AlembicWriter`, `ParticleFrame` | exercises the permanent-stub interface; `create()` returns None and consumer code branches cleanly |
| `log` | startup info + reset / pause / save / load events |
