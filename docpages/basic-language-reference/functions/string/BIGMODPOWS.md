\page BIGMODPOWS BIGMODPOW$ Function

```basic
BIGMODPOW$(string-expression, string-expression, string-expression)
```

Computes **modular exponentiation** on **arbitrary precision integers**, returning
((A^E) \bmod M) as a string.

---

### Examples

```basic
PRINT BIGMODPOW$("2", "10", "1000")
```

Produces `"24"`.

```basic
PRINT BIGMODPOW$("5", "3", "13")
```

Produces `"8"`.

```basic
PRINT BIGMODPOW$("123456789", "2", "100000")
```

Produces `"521"`.

---

### Notes

* All inputs must be valid base-10 integer strings.
* Efficient for large values (uses modular exponentiation, not naive power).
* Modulus must be non-zero.
* Negative bases are supported; exponent must be non-negative.
* Result is always in the range `0` to `M-1`.
* Result is returned as a decimal string.

---

**See also:**
\ref BIGMODINVS "BIGMODINV$" · \ref BIGMODS "BIGMOD$" · \ref BIGMULS "BIGMUL$" · \ref BIGADDS "BIGADD$"
