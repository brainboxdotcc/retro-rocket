\page ATAN ATAN Function

```basic
ATAN(real-expression)
```

Returns the **arc tangent** (inverse tangent) of the given expression, in **radians**.

The result will always lie between `-PI#/2` and `PI#/2`.

---

### Examples

```basic
PRINT ATAN(0)
```

Produces `0`.

```basic
PRINT ATAN(1)
```

Produces approximately `0.785398` (π/4).

```basic
PRINT ATAN(-1)
```

Produces approximately `-0.785398` (-π/4).

```basic
REM Validate a tangent identity
angle# = 0.3
PRINT ATAN(TAN(angle#))
```

Produces the original angle (within floating point accuracy).

---

### Notes

* Input range: any real number is valid.
* Return range: `-π/2 ≤ result ≤ π/2` radians.
* To convert the result to degrees, multiply by `180 / PI`.
* For quadrant-aware arc tangent with two arguments (x,y), use \ref ATAN2 "ATAN2".

---

**See also:**
\ref TAN "TAN" · \ref ATAN2 "ATAN2" · \ref ACS "ACS" · \ref ASN "ASN"
