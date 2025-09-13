\page TAN TAN Function

```basic
TAN(real-expression)
```

Returns the **tangent** of the given angle, where the angle is expressed in **radians**.

---

### Examples

```basic
PRINT TAN(0)
```

Produces `0`.

```basic
PRINT TAN(PI# / 4)
```

Produces approximately `1`.

```basic
REM Compute slope of a line from angle
angle# = RAD(30)
PRINT "Slope = "; TAN(angle#)
```

---

### Notes

* Argument must be in **radians**. Convert from degrees with:

  ```basic
  radians# = degrees * PI# / 180
  ```
* The tangent function has asymptotes where cosine is zero, i.e. at odd multiples of `PI#/2`. Calling `TAN` at these points will produce very large values or errors due to division by zero.
* Return value is a real number, unbounded in range.
* Inverse function: \ref ATAN "ATAN".

---

**See also:**
\ref SIN "SIN" · \ref COS "COS" · \ref ATAN "ATAN" · \ref ATAN2 "ATAN2"
