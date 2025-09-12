\page GETVARI GETVARI Function

```basic
GETVARI(string-expression)
```

Returns the **value of an integer variable** whose name is given in `string-expression`.

If the variable does not exist, an **error** is raised.
Use \ref EXISTSVARI "EXISTSVARI" to check for the variable’s existence first if you want to avoid an error.

---

### Examples

```basic
REM Direct retrieval of variable by name
SCORE = 9001
PRINT "Value of SCORE = "; GETVARI("SCORE")
```

```basic
REM Safe check before using
IF EXISTSVARI("LEVEL") THEN
    PRINT "LEVEL = "; GETVARI("LEVEL")
ELSE
    PRINT "LEVEL not defined"
ENDIF
```

```basic
REM Use with dynamic variable names
FOR i = 1 TO 3
    name$ = "VAL" + STR$(i)
    PRINT name$; " = ";
    IF EXISTSVARI(name$) THEN
        PRINT GETVARI(name$)
    ELSE
        PRINT "(undefined)"
    ENDIF
NEXT
```

---

### Notes

* The string must exactly match the variable’s name **without suffixes** (integer variables have no `$` or `#`).
* Raises an error if the variable has not been declared or assigned.
* Returns the current integer value as a 64-bit signed integer.
* To access reals or strings by name, use \ref GETVARR "GETVARR" or \ref GETVARS "GETVARS".
* Complements the \ref SETVARI "SETVARI" function for dynamic assignment.

---

**See also:**
\ref EXISTSVARI "EXISTSVARI" · \ref SETVARI "SETVARI" · \ref GETVARR "GETVARR" · \ref GETVARS "GETVARS"
