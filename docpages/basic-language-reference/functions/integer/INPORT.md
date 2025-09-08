\page INPORT INPORT Function
```basic
X = INPORT(port)
```

Reads a **byte** from an I/O `port` and returns it (0–255).


@note Only use on hardware that expects **8-bit** port reads at that address.

---

### How to read it

- Classic x86 **port-mapped I/O** (not MMIO).
- Often used for legacy controllers such as the PS/2 keyboard.

---

### Examples
```basic
REM Read data from PS/2 controller
DATA = INPORT(&60)
PRINT DATA
```

---

### Notes
- Always use the correct width for the target port.
- Continuous polling may stall the system; check status before reads if possible.

**See also:**  
\ref INPORTW "INPORTW" · \ref INPORTD "INPORTD" · \ref INPORTQ "INPORTQ" · \ref OUTPORT "OUTPORT"