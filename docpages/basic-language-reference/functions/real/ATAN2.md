\page ATAN2 ATAN2 Function

```basic
ATAN2(real-y, real-x)
```

Returns the **arc tangent of y ÷ x**, using the signs of both arguments to determine the correct **quadrant** of the angle.
The result is given in **radians**, in the range `-PI#` to `PI#`.

---

### Examples

```basic
PRINT ATAN2(1, 1)
```

Produces approximately `0.785398` (π/4).

```basic
PRINT ATAN2(1, -1)
```

Produces approximately `2.356194` (3π/4).

```basic
PRINT ATAN2(-1, -1)
```

Produces approximately `-2.356194` (-3π/4).

```basic
PRINT ATAN2(-1, 1)
```

Produces approximately `-0.785398` (-π/4).

```basic
REM Compute angle of a vector
dx# = 3
dy# = 4
angle# = ATAN2(dy#, dx#)
PRINT "Angle = "; angle#; " radians"
```

---

### Notes

* Unlike \ref ATAN "ATAN", which only returns results in `-π/2 … π/2`, `ATAN2` resolves the full circle by considering both `x` and `y` signs.
* Input range: both arguments may be any real numbers.
* Return range: `-π ≤ result ≤ π` radians.
* To convert to degrees, multiply by `180 / PI`.

---

**See also:**
\ref ATAN "ATAN" · \ref SIN "SIN" · \ref COS "COS" · \ref TAN "TAN"
