\page EXP EXP Function

```basic
EXP(real-expression)
```

Returns **e** (Euler’s number, approximately `2.71828`) raised to the power of the given expression.
In other words, `EXP(x)` = `e^x`.

---

### Examples

```basic
PRINT EXP(1)
```

Produces approximately `2.71828`.

```basic
PRINT EXP(0)
```

Produces `1`.

```basic
PRINT EXP(2)
```

Produces approximately `7.38906`.

```basic
REM Demonstrate inverse with LOG
x# = 5
PRINT "x = "; x#; "  log(exp(x)) = "; LOG(EXP(x#))
```

---

### Notes

* Argument may be any real number (positive, negative, or zero).
* Return value is always positive.
* For natural logarithm (inverse of EXP), use \ref LOG "LOG".

---

**See also:**
\ref LOG "LOG" · \ref SQR "SQR" · \ref POW "POW"
