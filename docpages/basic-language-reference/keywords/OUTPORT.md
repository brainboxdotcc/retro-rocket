\page OUTPORT OUTPORT Keyword
```basic
OUTPORT port, value
```

Writes the low **8 bits** of `value` to an I/O `port`.


@note Use the correct width for the target device; wrong widths can cause faults or misconfiguration.

---

### How to read it

- Sends one byte via port-mapped I/O.

---

### Examples
```basic
REM Reset the keyboard controller
OUTPORT &64, &FE
```

---

### Notes
- Some ports require delays or status checks between writes.
- Be cautious: writes may trigger immediate hardware actions.

**See also:**  
\ref OUTPORTW "OUTPORTW" · \ref OUTPORTD "OUTPORTD" · \ref OUTPORTQ "OUTPORTQ" · \ref INPORT "INPORT"