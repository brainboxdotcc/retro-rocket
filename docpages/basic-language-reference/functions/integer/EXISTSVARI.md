\page EXISTSVARI EXISTSVARI / EXISTSVARR / EXISTSVARS Functions

```basic
EXISTSVARI(string-expression)
EXISTSVARR(string-expression)
EXISTSVARS(string-expression)
```

Checks whether a variable with the given **name** currently exists in the BASIC program.
Returns `TRUE` if found, or `FALSE` otherwise.

* `EXISTSVARI` → checks for an **integer variable**.
* `EXISTSVARR` → checks for a **real (floating-point) variable**.
* `EXISTSVARS` → checks for a **string variable**.

The parameter is a **string** containing the variable’s name (without value, suffix only implied by function).

---

### Examples

```basic
REM Create a variable
A = 42

IF EXISTSVARI("A") THEN
    PRINT "Variable A exists"
ENDIF
```

```basic
REM Example with strings
B$ = "Hello"

IF EXISTSVARS("B$") THEN
    PRINT "Variable B$ exists"
ENDIF
```

```basic
REM Useful for checking before SETVAR
varName$ = "SCORE"
IF EXISTSVARI(varName$) = FALSE THEN
    PRINT "Variable not yet created"
ENDIF
```

---

### Notes

* Variable name must match exactly, including suffix (`$` for strings, `#` for reals).
* These functions are typically used with \ref SETVARI "SETVARI", \ref SETVARR "SETVARR", and \ref SETVARS "SETVARS" to safely determine if a variable can be set or referenced dynamically.
* Existence check does not return the value; it only reports if the variable has been **declared or used**.
* Using `EXISTSVAR*` avoids runtime errors when handling dynamically named variables.

---

**See also:**
\ref SETVARI "SETVARI" · \ref SETVARR "SETVARR" · \ref SETVARS "SETVARS"
