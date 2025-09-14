\page type-integer Integer Variables

Integer variables are represented by any variable **without a suffix**, e.g. `A` or `MYVAR`.
All integers in Retro Rocket BASIC are **64-bit signed values**.

---

### Examples

```basic
A = 42
B = -1000000
PRINT A + B
```

```basic
REM Loop using an integer counter
FOR i = 0 TO 9
    PRINT "i = "; i
NEXT
```

---

### Notes

* Range: `-9,223,372,036,854,775,808` to `9,223,372,036,854,775,807`.
* Arithmetic wraps around on overflow.
* When dividing integers, fractional results are discarded unless the value is stored in a real (`#`) variable.
* To explicitly ensure integer division, use \ref DIV "DIV".

---

**See also:**
\ref type-real "Real Variables" · \ref type-string "String Variables" · \ref type-array "Array Variables"
