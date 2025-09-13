\page DEG DEG Function

```basic
DEG(real-expression)
```

Converts an angle from **radians** to **degrees**.
The result is a real number.

---

### Examples

```basic
PRINT DEG(PI#)
```

Produces `180`.

```basic
PRINT DEG(PI# / 2)
```

Produces `90`.

```basic
REM Round-trip conversion
angle# = 45
PRINT "Radians = "; RAD(angle#)
PRINT "Degrees = "; DEG(RAD(angle#))
```

---

### Notes

* Conversion formula: `degrees = radians * 180 / PI`.
* Returned as a real value; may not be an integer if the input is not an exact multiple of π/180.
* Useful for displaying angles calculated in radians to users in degrees.

---

**See also:**
\ref RAD "RAD" · \ref SIN "SIN" · \ref COS "COS" · \ref TAN "TAN"
