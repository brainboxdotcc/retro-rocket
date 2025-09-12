\page INT INT Function

```basic
INT(real-expression)
```

Truncates a **real number** to an **integer** by discarding the fractional part (i.e. truncates **towards zero**).

---

### Examples

```basic
PRINT INT(3.99)
```

Produces `3`.

```basic
PRINT INT(-7.25)
```

Produces `-7`.

```basic
REM Convert user input to an integer by truncation
INPUT "Enter a real number > " ; val#
PRINT "Truncated = "; INT(val#)
```

```basic
REM Use INT to derive an index from a real value
val# = 12.9
idx = INT(val# / 4.0)
PRINT "Index = "; idx
```

---

### Notes

* Returns a Retro Rocket BASIC integer (64-bit signed).
* **Differs from BBC BASIC:** in BBC BASIC `INT` is a floor (e.g. `INT(-7.25) = -8`). In Retro Rocket BASIC, `INT(-7.25) = -7` (truncate towards zero).
* To get the fractional part:

  ```basic
  frac# = val# - INT(val#)
  ```

---

**See also:**
\ref ABS "ABS" · \ref SGN "SGN"
