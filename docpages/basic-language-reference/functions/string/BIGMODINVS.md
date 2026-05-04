\page BIGMODINVS BIGMODINV$ Function

```basic
BIGMODINV$(string-expression, string-expression)
```

Computes the **modular multiplicative inverse** of an **arbitrary precision integer**.
Returns a value (X) such that:

```
(A * X) MOD M = 1
```

---

### Examples

```basic
PRINT BIGMODINV$("3", "11")
```

Produces `"4"`.

```basic
PRINT BIGMODINV$("10", "17")
```

Produces `"12"`.

```
(10 * 12) MOD 17 = 1
```

---

### Notes

* Inputs must be valid base-10 integer strings.
* The inverse exists only if `A` and `M` are coprime.
* If no inverse exists, an error is produced.
* Modulus must be non-zero.
* Result is returned as a decimal string.

---

**See also:**
\ref BIGMODPOWS "BIGMODPOW$" · \ref BIGMODS "BIGMOD$" · \ref BIGGCDS "BIGGCD$" · \ref BIGMULS "BIGMUL$"
