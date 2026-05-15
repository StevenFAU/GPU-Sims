# Fenced upstream-citation example

This file demonstrates that cat1.upstream-citation skips citations
inside fenced code blocks. The example below shows a deliberately
version-mismatched citation -- but it is illustrative content,
not a real citation, so the check must not fire.

## Example: doc-block syntax for upstream citations

```cpp
// Cubic spline kernel: SPlisHSPlasH 1.8.10 SPHKernels.h:43-78
// (version 1.8.10 deliberately fabricated for this example;
// real anchor is 2.16.1 per ground-truth-sources.md)
```

End of file.
