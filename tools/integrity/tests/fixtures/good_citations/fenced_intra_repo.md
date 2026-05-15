# Fenced intra-repo citation example

This file demonstrates that cat1.intra-repo skips citations inside
fenced code blocks. The fixture below contains a deliberately
dangling citation -- but it is illustrative content, not a real
citation, so the check must not fire.

## Example: terminal output with a bare path citation

```
$ ./run_example
warning: nonexistent_file.cpp:42 not found
```

End of file.
