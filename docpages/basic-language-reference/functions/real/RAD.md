\page RAD RAD Function

```basic
RAD(real-expression)
```

Converts an angle from **degrees** to **radians**.
The result is a real number.

---

### Examples

```basic
PRINT RAD(180)
```

Produces approximately `3.14159` (`PI#`).

```basic
PRINT RAD(90)
```

Produces approximately `1.570796`.

```basic
REM Round-trip conversion
angle# = 270
PRINT "Radians = "; RAD(angle#)
PRINT "Degrees = "; DEG(RAD(angle#))
```

---

### Notes

* Conversion formula: `radians = degrees * PI / 180`.
* Returned as a real value.
* Useful for trigonometric functions (\ref SIN "SIN", \ref COS "COS", \ref TAN "TAN"), which all expect input in radians.

---

**See also:**
\ref DEG "DEG" · \ref SIN "SIN" · \ref COS "COS" · \ref TAN "TAN"
