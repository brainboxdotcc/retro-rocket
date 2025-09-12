\page OCTVAL OCTVAL Function

```basic
OCTVAL(string-expression)
```

Returns the **integer value** of a string containing an **octal number**.
The string must consist only of valid octal digits (`0–7`).

---

### Examples

```basic
PRINT OCTVAL("10")
```

Produces `8`.

```basic
PRINT OCTVAL("755")
```

Produces `493`.

```basic
REM Convert user input
INPUT "Enter an octal number > " ; o$
val = OCTVAL(o$)
PRINT "You entered decimal "; val
```

```basic
REM Round-trip using OCT$
n = 64
o$ = OCT$(n)
PRINT o$; " = "; OCTVAL(o$)
```

---

### Notes

* Parsing is strict: only digits `0–7` are allowed.
* Returns a **64-bit integer**.
* If the string is empty, or contains any character outside `0–7`, the return value is `0`.
* **Important distinction:**

  * In BASIC source code, you can write octal literals with a `&O` prefix (e.g. `&O755`).
  * In strings passed to `OCTVAL`, only the raw digits are valid — no prefix.

---

**See also:**
\ref OCT "OCT\$" · \ref STR "STR\$" · \ref HEXVAL "HEXVAL"
