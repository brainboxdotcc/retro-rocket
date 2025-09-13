\page GETVARR GETVARR Function

```basic
GETVARR(string-expression)
```

Retrieves the value of a **real (floating-point) variable** by its **name**.
If the variable does not exist, an error is thrown.
Use \ref EXISTSVARR "EXISTSVARR" to check for existence without triggering an error.

---

### Examples

```basic
REM Define a real variable
temperature# = 21.5

REM Retrieve it by name
PRINT GETVARR("temperature#")
```

Produces `21.5`.

```basic
REM Attempting to get a missing variable
PRINT GETVARR("doesnotexist#")
```

Raises an error.

```basic
REM Safe access using EXISTSVARR
IF EXISTSVARR("speed#") THEN
    PRINT "Speed = "; GETVARR("speed#")
ELSE
    PRINT "Speed not defined"
ENDIF
```

---

### Notes

* The string must exactly match the variable name, including the `#` suffix for real variables.
* If the name refers to a variable of a different type (integer or string), an error is raised.
* Returns a 64-bit double-precision floating-point value.

---

**See also:**
\ref GETVARI "GETVARI" · \ref GETVARSTRING "GETVARS$" · \ref EXISTSVARI "EXISTSVARR"
