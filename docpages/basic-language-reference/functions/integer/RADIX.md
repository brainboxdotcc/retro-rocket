\page RADIX RADIX Function

```basic
RADIX(string-expression, integer-expression)
```

Converts a **string** (first parameter) to an **integer** using the specified **radix** (base).
The second parameter is the base, typically `2` (binary), `8` (octal), `10` (decimal), or `16` (hexadecimal).

---

### Examples

```basic
PRINT RADIX("1010", 2)
```

Produces `10` (binary → decimal).

```basic
PRINT RADIX("755", 8)
```

Produces `493` (octal → decimal).

```basic
PRINT RADIX("1234", 10)
```

Produces `1234`.

```basic
PRINT RADIX("FF", 16)
```

Produces `255`.

---

### Notes

* The string must consist only of valid digits for the given base.

  * Binary: `0–1`
  * Octal: `0–7`
  * Decimal: `0–9`
  * Hexadecimal: `0–9`, `A–F`, `a–f`
* Returns a **64-bit integer**.
* Invalid input (invalid digit for base, or empty string) returns `0`.
* Equivalent to \ref HEXVAL "HEXVAL" or \ref OCTVAL "OCTVAL", but generalised for any supported base.

---

**See also:**
\ref HEXVAL "HEXVAL" · \ref OCTVAL "OCTVAL" · \ref STR "STR\$"
