\page ACS ACS Function

```basic
ACS(real-expression)
```

Returns the **arc cosine** (inverse cosine) of the given expression, in **radians**.
The input must be between `-1` and `1`.

---

### Examples

```basic
PRINT ACS(1)
```

Produces `0`.

```basic
PRINT ACS(0)
```

Produces approximately `1.570796` (π/2).

```basic
PRINT ACS(-1)
```

Produces approximately `3.141593` (π).

```basic
REM Validate a cosine identity
angle# = 0.75
PRINT ACS(COS(angle#))
```

Produces the original angle (within floating point accuracy).

---

### Notes

* Argument range: `-1 ≤ x ≤ 1`. Values outside this range cause an error.
* Return range: `0 ≤ result ≤ π` radians.
* To convert the result to degrees, multiply by `180 / PI`.
* Inverse functions:

  * \ref ASN "ASN" (arc sine)
  * \ref ATN "ATN" (arc tangent)

---

**See also:**
\ref COS "COS" · \ref ASN "ASN" · \ref ATN "ATN" · \ref TAN "TAN"
