\page COS COS Function

```basic
COS(real-expression)
```

Returns the **cosine** of the given angle, where the angle is expressed in **radians**.

---

### Examples

```basic
PRINT COS(0)
```

Produces `1`.

```basic
PRINT COS(PI# / 2)
```

Produces approximately `0`.

```basic
PRINT COS(PI#)
```

Produces `-1`.

```basic
REM Plot cosine values for multiples of 45 degrees
FOR d = 0 TO 360 STEP 45
    r# = d * PI# / 180
    PRINT "COS("; d; "°) = "; COS(r#)
NEXT
```

---

### Notes

* Argument is in **radians**. Convert degrees to radians with:

  ```basic
  radians# = degrees * PI# / 180
  ```
* Return value is a real number between `-1` and `1`.
* Related inverse function: \ref ACS "ACS".

---

**See also:**
\ref SIN "SIN" · \ref TAN "TAN" · \ref ACS "ACS" · \ref ATAN "ATAN"
