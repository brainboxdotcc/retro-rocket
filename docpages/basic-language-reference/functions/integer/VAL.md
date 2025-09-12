\page VAL VAL Function

```basic
VAL(string-expression)
```

Converts a **string** containing a number into an **integer value**.
The string may contain digits `0–9` and an optional leading `+` or `-`.

---

### Examples

```basic
PRINT VAL("1234")
```

Produces `1234`.

```basic
PRINT VAL("-42")
```

Produces `-42`.

```basic
REM Convert user input
INPUT "Enter a number > " ; n$
x = VAL(n$)
PRINT "You entered "; x
```

```basic
REM Strings that are not numbers
PRINT VAL("hello")
```

Produces `0`.

---

### Notes

* Returns a 64-bit signed integer.
* If the string is empty or contains invalid characters, the return value is `0`.
* Only integer values are supported — fractional values are truncated or treated as invalid.
* For parsing hexadecimal or octal strings, see \ref HEXVAL "HEXVAL" and \ref OCTVAL "OCTVAL".

---

**See also:**
\ref STR "STR$" · \ref HEXVAL "HEXVAL" · \ref OCTVAL "OCTVAL" · \ref RADIX "RADIX"
