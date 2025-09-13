\page ASN ASN Function

```basic
ASN(real-expression)
```

Returns the **arc sine** (inverse sine) of the given expression, in **radians**.
The input must be between `-1` and `1`.

---

### Examples

```basic
PRINT ASN(0)
```

Produces `0`.

```basic
PRINT ASN(1)
```

Produces approximately `1.570796` (π/2).

```basic
PRINT ASN(-1)
```

Produces approximately `-1.570796` (-π/2).

```basic
REM Validate a sine identity
angle# = 0.5
PRINT ASN(SIN(angle#))
```

Produces the original angle (within floating point accuracy).

---

### Notes

* Argument range: `-1 ≤ x ≤ 1`. Values outside this range cause an error.
* Return range: `-π/2 ≤ result ≤ π/2` radians.
* To convert the result to degrees, multiply by `180 / PI`.
* Inverse functions:

  * \ref ACS "ACS" (arc cosine)
  * \ref ATN "ATN" (arc tangent)

---

**See also:**
\ref SIN "SIN" · \ref ACS "ACS" · \ref ATN "ATN" · \ref TAN "TAN"
