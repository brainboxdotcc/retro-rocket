\page BOOL BOOL$ Function

```basic
string-value = BOOL$(numeric-value)
```

Returns the string form of a boolean value.

* Any non-zero numeric value becomes `TRUE`.
* Any zero numeric value becomes `FALSE`.

---

**Notes**

* Works with both integer and real values.
* Negative values are also treated as `TRUE`.

---

**Errors**

* None.

---

**Examples**

```basic
PRINT BOOL$(0)      : REM "FALSE"
PRINT BOOL$(42)     : REM "TRUE"
PRINT BOOL$(-7)     : REM "TRUE"
```


