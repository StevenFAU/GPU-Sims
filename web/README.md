# web/

Reserved for the future portfolio website.

**Status:** Empty by design. Per [`../docs/overarching-spec.md`](../docs/overarching-spec.md) §10, the website is built after several demos are shipping — not on day one.

## Planned content

When this directory is populated, it will hold either a hand-rolled static site (Astro or Next.js) or a generated design with manual integration. The site will:

- Embed Stack B (WebGPU) demos directly via canvas elements.
- Show GIF / video previews and offline-rendered hero stills for Stack C and Stack D demos.
- Link to GitHub source and per-sim READMEs.
- Briefly explain the underlying mathematics of each sim.

## Why this is empty

The first several sims need to ship before the website has content to showcase. Building the site before there are demos to embed produces a website about a website. Building it after at least 3–5 sims are running gives the design real content to anchor against.

The motivation for reserving the directory now is so that when the site work begins, the folder is in the expected place and CI / deploy configuration can be added without restructuring the repo.

## Domain

To be decided. The eventual portfolio site will live at a personal domain (TBD); the GitHub repo lives at [`github.com/StevenFAU/GPU-Sims`](https://github.com/StevenFAU/GPU-Sims).
