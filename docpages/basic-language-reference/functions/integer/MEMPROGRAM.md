\page MEMPROGRAM MEMPROGRAM Function

```basic
MEMPROGRAM
```

Returns the current amount of memory (in bytes) used by the **private heap** of the current BASIC program.
This reflects the memory actively allocated right now, not the peak or global system usage.

---

### Examples

```basic
REM Show memory used by this program
PRINT "Program is using "; MEMPROGRAM; " bytes"
```

For a large BASIC program this might print:

```
Program is using 401920 bytes
```

```basic
REM Compare with peak usage
PRINT "Currently used = "; MEMPROGRAM; " bytes"
PRINT "Peak used      = "; MEMPEAK; " bytes"
```

```basic
REM Report usage in kilobytes
PRINT "Program heap = "; MEMPROGRAM / 1024; " KB"
```

Might print:

```
Program heap = 392 KB
```

---

### Notes

* Value is specific to the **current BASIC program’s private heap**.
* Typically in the range of a few hundred kilobytes for larger programs.
* Use \ref MEMPEAK "MEMPEAK" to see the maximum ever reached since program start.
* Returned as a 64-bit integer.

---

**See also:**
\ref MEMPEAK "MEMPEAK" · \ref MEMUSED "MEMUSED" · \ref MEMFREE "MEMFREE" · \ref MEMORY "MEMORY"
