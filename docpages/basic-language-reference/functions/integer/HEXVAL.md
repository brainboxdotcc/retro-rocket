\page HEXVAL HEXVAL Function

```basic
integer-value = HEXVAL(string-value)
```

Returns the **integer value** of a string containing a **hexadecimal number**.
The string must consist only of valid hex digits (`0–9`, `A–F`, `a–f`).

---

### Examples

```basic
PRINT HEXVAL("FF")
```

Produces `255`.

```basic
PRINT HEXVAL("deadbeef")
```

Produces `3735928559`.

```basic
REM Convert user input
INPUT "Enter a hex number > " ; h$
val = HEXVAL(h$)
PRINT "You entered decimal "; val
```

```basic
REM Round-trip using HEX$
n = &2A        REM hex literal in source
h$ = HEX$(n)
PRINT h$; " = "; HEXVAL(h$)
```

---

### Notes

* Parsing is **case-insensitive**: `"FF"`, `"Ff"`, and `"ff"` all return `255`.
* Returns a **64-bit integer**.
* If the string is empty, or contains any non-hex character, the return value is `0`.

---

**See also:**
\ref HEX "HEX$" · \ref STR "STR$"
