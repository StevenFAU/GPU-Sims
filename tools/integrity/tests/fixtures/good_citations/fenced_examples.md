<!-- Fenced annotation examples fixture.

This file contains intentionally-malformed annotation grammar strings
inside fenced code blocks. The cat1.annotation-form check must skip
these -- they are documentation examples, not live annotations. -->

# Fenced annotation examples

## Example 1: malformed grammar inside Python fence

```python
# This is a malformed annotation example:
# integrity-allow: malformed-grammar-here-no-semicolons
def foo():
    pass
```

## Example 2: blanket `*` inside generic fence

```
// integrity-allow: *; this is blanket and would be invalid; n/a
struct Bar;
```

## Example 3: short reason inside fence

```cpp
// integrity-allow: cat1.intra-repo; tooshort; n/a
int x;
```

End of file.
