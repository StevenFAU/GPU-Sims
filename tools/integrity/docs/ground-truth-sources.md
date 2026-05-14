# Ground-truth sources for the integrity toolkit

Per spec Appendix A. Adding a source requires:

1. Vendoring the upstream under `references/<UpstreamName>/`, OR documenting
   an algebraic derivation under `tools/integrity/docs/algebraic/`
2. Pinning the anchor (version + SHA)
3. Updating the TOML block below
4. Updating the relevant check(s) to consume the new source

## v1 registry

The block below is parsed by `cat1_citations/upstream_anchor.py`. Everything
outside the fenced TOML block is prose for humans.

```toml
[SPlisHSPlasH]
anchor_version = "2.16.1"
anchor_sha     = "6bff55a6eaf14083d34650f22a268ce156b62b54"
vendor_root    = "references/SPlisHSPlasH"
anchor_doc     = ".gitignore"
upstream_url   = "https://github.com/InteractiveComputerGraphics/SPlisHSPlasH"
used_by_checks = ["cat1.upstream-citation", "cat1.upstream-anchor", "cat3.cubic-kernel"]
```

## Notes on v1 registry contents

- **SPlisHSPlasH:** Vendored at Phase 11.5 setup-1 after the original
  fabricated `1.8.10` anchor was found non-existent. See
  `docs/diagnostics/_audits/phase11_5_setup1_2026-05-14_setup1.md`.

## Not yet registered (intentional)

- **Chakazul/Lenia (LeniaNDK):** Cited in `continuous-ca/lenia-fft/python/lenia_fft/presets.py:11`
  but not vendored. Per the Layer 3 lenia audit
  (`sims_lenia_chakazul_2026-05-14_architect3b.md`) the upstream master
  resolves cleanly but the citation is master-HEAD-only (no historical
  pin). Adding LeniaNDK to this registry requires vendoring it under
  `references/Chakazul-Lenia/` at a chosen anchor SHA. Deferred — this
  is the test case for `cat1.unregistered-upstream`.
