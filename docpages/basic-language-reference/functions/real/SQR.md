\page SQR SQR Function

```basic
SQR(real-expression)
```

Returns the **square root** of the given expression.

---

### Examples

```basic
PRINT SQR(9)
```

Produces `3`.

```basic
PRINT SQR(2)
```

Produces approximately `1.414213`.

```basic
REM Validate identity: SQR(x^2) = ABS(x)
x# = -7
PRINT "SQR("; x#; "^2) = "; SQR(x# * x#)
PRINT "ABS("; x#; ") = "; ABS(x#)
```

---

### Notes

* Argument must be a non-negative real number.
* Passing a negative value results in an error.
* Return value is a real number.
* For arbitrary powers (including fractional), use \ref POW "POW".

---

**See also:**
\ref POW "POW" · \ref EXP "EXP" · \ref ABS "ABS"
