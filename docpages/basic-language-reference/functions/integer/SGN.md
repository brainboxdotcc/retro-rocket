\page SGN SGN Function

```basic
SGN(integer-expression)
```

Returns the **sign** of an integer expression:

* `-1` if the expression is negative
* `0` if the expression is zero
* `1` if the expression is positive

---

### Examples

```basic
PRINT SGN(-42)
```

Produces `-1`.

```basic
PRINT SGN(0)
```

Produces `0`.

```basic
PRINT SGN(1234)
```

Produces `1`.

```basic
REM Use SGN in a comparison
balance = -150
IF SGN(balance) = -1 THEN
    PRINT "Account overdrawn!"
ENDIF
```

---

### Notes

* Input must be an integer expression.
* Return value is always `-1`, `0`, or `1`.
* Can be used in compact conditionals or arithmetic to simplify sign handling.

---

**See also:**
\ref ABS "ABS" · \ref INT "INT" · \ref ROUND "ROUND"
