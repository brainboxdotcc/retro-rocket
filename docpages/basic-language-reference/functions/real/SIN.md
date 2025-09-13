\page SIN SIN Function

```basic
SIN(real-expression)
```

Returns the **sine** of the given angle, where the angle is expressed in **radians**.

---

### Examples

```basic
PRINT SIN(0)
```

Produces `0`.

```basic
PRINT SIN(PI# / 2)
```

Produces `1`.

```basic
PRINT SIN(PI#)
```

Produces approximately `0`.

```basic
REM Compute vertical component of a vector
length# = 10
angle# = RAD(30)
dy# = length# * SIN(angle#)
PRINT "Vertical component = "; dy#
```

---

### Notes

* Argument must be in **radians**. Convert from degrees with:

  ```basic
  radians# = degrees * PI# / 180
  ```
* Result is a real number between `-1` and `1`.
* Inverse function: \ref ASN "ASN".

---

**See also:**
\ref COS "COS" · \ref TAN "TAN" · \ref ASN "ASN" · \ref ATAN "ATAN"
