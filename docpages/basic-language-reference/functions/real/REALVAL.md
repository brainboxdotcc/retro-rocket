\page REALVAL REALVAL Function

```basic
real-value = REALVAL(string-value)
```

Converts a string containing a number into a **real (floating-point)** value.

---

### Examples

```basic
PRINT REALVAL("3.14159")
```

Produces `3.14159`.

```basic
PRINT REALVAL("-42.75")
```

Produces `-42.75`.

```basic
REM Using REALVAL for user input
INPUT str$
num# = REALVAL(str$)
PRINT "You entered "; num#
```

---

### Notes

* Accepts decimal and optionally signed values.
* If the string cannot be parsed as a valid real number, an error is raised.
* Returns a 64-bit double-precision floating-point value.
* For integer conversion, use \ref VAL "VAL".

---

**See also:**
\ref VAL "VAL" · \ref HEXVAL "HEXVAL" · \ref OCTVAL "OCTVAL" · \ref RADIX "RADIX"
