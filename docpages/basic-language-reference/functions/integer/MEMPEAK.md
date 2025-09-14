\page MEMPEAK MEMPEAK Function

```basic
MEMPEAK
```

Returns the **peak memory usage** (in bytes) of the current BASIC program’s **private heap**.
This is the largest amount of memory the program has allocated since it started running.

---

### Examples

```basic
REM Display peak usage directly
PRINT "Peak memory usage = "; MEMPEAK; " bytes"
```

For a typical program this might print:

```
Peak memory usage = 409600 bytes
```

```basic
REM Report in kilobytes
PRINT "Peak memory = "; MEMPEAK / 1024; " KB"
```

Might print:

```
Peak memory = 400 KB
```

```basic
REM Compare with current usage
PRINT "Currently used = "; MEMUSED; " bytes"
PRINT "Peak usage     = "; MEMPEAK; " bytes"
```

---

### Notes

* The value is specific to the **current BASIC program**, not global OS memory usage.
* It is a **high-water mark**: once reached, the peak value will not decrease, even if memory is freed.
* Typical Retro Rocket BASIC programs today peak around a few hundred kilobytes, not megabytes.
* Returned as a 64-bit integer.

---

**See also:**
\ref MEMUSED "MEMUSED" · \ref MEMFREE "MEMFREE" · \ref MEMORY "MEMORY" · \ref MEMORY "MEMRELEASE" · \ref MEMORY "MEMALLOC"
