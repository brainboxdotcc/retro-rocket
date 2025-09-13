\page GETVAR GETVAR$ Function

```basic
GETVAR$(string-expression)
```

Retrieves the value of a **string variable** by its **name**.
If the variable does not exist, an error is thrown.
Use \ref EXISTSVARS "EXISTSVARS" to check for existence without triggering an error.

---

### Examples

```basic
REM Define a string variable
username$ = "Alice"

REM Retrieve it by name
PRINT GETVAR$("username$")
```

Produces `Alice`.

```basic
REM Attempting to get a missing variable
PRINT GETVAR$("doesnotexist$")
```

Raises an error.

```basic
REM Safe access with EXISTSVARS
IF EXISTSVARS("config$") THEN
    PRINT "Config = "; GETVAR$("config$")
ELSE
    PRINT "Config variable not set"
ENDIF
```

---

### Notes

* The string must exactly match the variable name, including the `$` suffix for string variables.
* If the name refers to a variable of a different type (integer or real), an error is raised.
* Returns a null-terminated string.

---

**See also:**
\ref GETVARI "GETVARI" · \ref GETVARR "GETVARR" · \ref EXISTSVARS "EXISTSVARS"
