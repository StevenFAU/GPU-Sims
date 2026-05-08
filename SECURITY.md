# Security Policy

## Scope

GPU-Sims is a research and portfolio repository. It contains GPU simulation code intended for local development and demonstration; it does not run as a network service, store user data, or process untrusted input from the internet.

The primary security considerations are:

- **Reference-implementation licensing.** Any borrowed code is checked for license compatibility before inclusion. See per-sim `README.md` files for attribution.
- **Shader and compute correctness.** Bugs in compute shaders that read out of bounds or write to unexpected memory are functional bugs, not security vulnerabilities, but are taken seriously and tracked as bugs.
- **Dependency hygiene.** Public dependencies pulled in via CMake / npm / pip should be pinned and reviewed before update.

## Reporting

If you believe you have found a security-relevant issue (e.g., a malicious dependency, a license violation, or a code path that handles untrusted input unsafely), please open a private security advisory through GitHub:

> [github.com/StevenFAU/GPU-Sims/security/advisories](https://github.com/StevenFAU/GPU-Sims/security/advisories)

For non-security bugs, file a regular issue.
