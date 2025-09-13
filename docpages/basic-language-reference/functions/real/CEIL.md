\page CEIL CEIL Function

```basic
CEIL(real-expression)
```

Rounds the provided real value **upwards** to the nearest integer.
The result is the smallest integer greater than or equal to the input.

---

### Examples

```basic
PRINT CEIL(3.2)
```

Produces `4`.

```basic
PRINT CEIL(-3.2)
```

Produces `-3`.

```basic
REM Use CEIL to calculate number of pages needed
items = 23
perPage = 10
pages = CEIL(items / perPage)
PRINT "Pages required = "; pages
```

---

### Notes

* Always rounds **towards positive infinity**.
* Return type is a 64-bit integer.
* Differs from \ref INT "INT", which truncates towards zero, and \ref ROUND "ROUND", which rounds to the nearest integer.

---

**See also:**
\ref FLOOR "FLOOR" · \ref INT "INT" · \ref ROUND "ROUND"
