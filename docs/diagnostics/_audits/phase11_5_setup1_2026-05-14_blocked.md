---
title: Phase 11.5 Setup-1 — BLOCKED at Step 1 (target tag does not exist upstream)
date: 2026-05-14
author: setup1
phase: 11.5
status: blocked
scope: read-only-network
---

> Setup-1 halted at Step 1. The SPlisHSPlasH tag `1.8.10` (and its `v1.8.10` variant) cited in `particle-fluids/sph-water/docs/load-bearing-decisions.md:8-9` as the formula-citation anchor **does not exist in the upstream repository**. The git remote was probed without cloning. No clone was attempted. No file was modified other than this report. Explicit instruction on which tag to substitute is required before proceeding to Step 2.

## Section A: Step 1 — Network and tag probe

### A.1 Preflight

Network and git checks passed:

```
$ git --version
git version 2.43.0

$ curl --silent --head https://github.com/InteractiveComputerGraphics/SPlisHSPlasH.git | head -1
HTTP/2 301
```

The `HTTP/2 301` is the expected redirect to the canonical URL — GitHub serves a redirect for the bare repo URL. The clone command in Step 2 does not depend on this URL form, so the redirect is not a blocker.

### A.2 Tag-existence probe (no clone)

The brief-specified filter pattern `'refs/tags/(1\.8|v1\.8)'` returned **zero matches**:

```
$ git ls-remote --tags https://github.com/InteractiveComputerGraphics/SPlisHSPlasH.git \
    | grep -E 'refs/tags/(1\.8|v1\.8)' | head
(no output)
```

The full tag enumeration shows why — upstream jumps from the `1.x` line directly to `2.x`, with no `1.4`–`1.9` series ever published:

```
$ git ls-remote --tags https://github.com/InteractiveComputerGraphics/SPlisHSPlasH.git
139e5fd0e4f0ace801f039ab065a2e5ee72f8f9f	refs/tags/1.1.0
3178e97a6377d77363175f4b97c5012cd0a537b7	refs/tags/1.2.0
e7cd4d341a55bf2ad4d484e38f53aca9926a2f5b	refs/tags/1.3.0
eb935496b2365b116716cfce1a016a48d53b1e35	refs/tags/1.3.1
f7cc42196b51ffd3bb640db4f7ab69d9b06248d7	refs/tags/2.0.0
924cfc454a2402eaf42c3e80d7b41cd11cc3bb09	refs/tags/2.1.0
d0c28bba53f9b9e35a94104dbba78b5a5cceff2a	refs/tags/2.10.0
fd57b7528a6862aa55bcd29b00a5c63f3b043743	refs/tags/2.11.0
2874d0eda8fd7da7e69fb2aee249897c5b76fb99	refs/tags/2.12.0
e9e67e40884476d8c65f2447bf10ba4aa14cc801	refs/tags/2.13.0
8c5e56a6b5a11e3e6755e0d03f938b8100f69be3	refs/tags/2.15.0
6bff55a6eaf14083d34650f22a268ce156b62b54	refs/tags/2.16.1
93001ad7b73d4f253be9ca4c1e68295a9899cbd2	refs/tags/2.2.0
7a7f4e712c0316fa5e771a6576f5336a32a11ab7	refs/tags/2.3.0
c78e738d441dded16335007129eb33f1d08024c1	refs/tags/2.4.0
ee26ef34e504191125997fc343aa2b05743c8891	refs/tags/2.5.0
ffe47dcf2ff66659ab132e213867b5912bab76dc	refs/tags/2.6.0
20bef647c69b28d0c7a35e57296c34e4c9c59a09	refs/tags/2.7.0
48011bcd321b6ad73d633795b3efbf7388ae37ad	refs/tags/2.8.0
9937dd28dbeb7a0fdccb94878e72fa089f0b6e67	refs/tags/2.8.1
a4ff5c46df62de4c36c63f9b54d8a91bbb79f94e	refs/tags/2.8.2
7c341ffcd25b57f61096384f171451459f6c7840	refs/tags/2.8.7
63f16d7d7834f24bd8604b906e4f3c02b04c738a	refs/tags/2.9.0
```

### A.3 Closest tags to the requested `1.8.10`

There is no version-numeric "closest" since the `1.4`–`1.9` range was never tagged. The structural neighbors are:

- **Latest `1.x` ever published:** `1.3.1` (SHA `eb935496b2365b116716cfce1a016a48d53b1e35`). End of the 1.x branch.
- **First `2.x`:** `2.0.0` (SHA `f7cc42196b51ffd3bb640db4f7ab69d9b06248d7`). The version line continues from here.
- **Most recent overall:** `2.16.1` (SHA `6bff55a6eaf14083d34650f22a268ce156b62b54`).

### A.4 Implications for the SHA in `load-bearing-decisions.md`

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
The doc claims SHA `c254caf2705ebf5271408dd37a091aa379258a38`. None of the 22 tags listed above match. The doc author flagged this SHA as suspicious in the brief (matches the Alembic SHA elsewhere in the same file). Combined with the non-existent tag, **both the tag and the SHA in `load-bearing-decisions.md:8-9` are unverifiable against the upstream remote.** This does not invalidate the formula-citation work — the line-numbered citations in the DFSPH shaders point at whatever upstream commit was actually consulted by their author; we just do not yet know which commit that was.

## Section B: What was NOT done

- No clone. `references/` does not exist; was never created.
- `.gitignore` was NOT modified.
- No shader file, no markdown file other than this report, no CMake, no source.
- No build, no binary run.

`git status` should show only the pre-existing modified files (`gpu_profiler.{hpp,cpp}`, `vk/context.cpp`) plus this new audit file under `docs/diagnostics/_audits/`.

## Section C: Decision needed before Setup-1 can resume

The brief's last sentence in Step 1 is binding: *"If neither `1.8.10` nor `v1.8.10` appears in the ls-remote output, report the closest tags found and STOP. Do not proceed without explicit instruction on which tag to use."*

Candidate substitute tags, with the tradeoff for each:

1. **`2.0.0`** (SHA `f7cc42196b51ffd3bb640db4f7ab69d9b06248d7`) — the earliest version that exists post-1.3.1, likely closest in time / API surface to what the original shader-citation author was reading if they misremembered "1.8.10" as a real label. Risk: still potentially a different API generation than the citations target.
2. **`1.3.1`** (SHA `eb935496b2365b116716cfce1a016a48d53b1e35`) — last actual 1.x release. Lowest version that exists. Risk: predates 1.8.10 by an unknowable margin; if shader citations target a later API, all line numbers will drift.
3. **Some `2.x` release** (e.g. `2.16.1`, latest) — newest. Risk: highest API drift relative to whatever was consulted in 2024-25 when these shaders were written.
4. **A specific commit SHA** — if the original consultation commit can be recovered from elsewhere (git history, earlier audit, author memory), pin to that exact SHA instead of a tag.

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
Recommend the user inspect `particle-fluids/sph-water/docs/phase11_sph_water.md` § 2 (which `load-bearing-decisions.md:3-4` defers to) and any commit-history surrounding the shader docblock authorship — that material may reveal which upstream snapshot the line-number citations actually came from. Until then, Step 2 of Setup-1 cannot proceed deterministically.

## Section D: Resumption

Once a tag (or commit SHA) is chosen, Setup-1 can resume at Step 2 with the clone command modified to use the chosen reference. Steps 3-6 are independent of which tag is chosen and will execute identically.
