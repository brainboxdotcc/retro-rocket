\page PEEKQ PEEKQ Function
```basic
X = PEEKQ(address)
```

Reads a **64-bit quad-word** from `address` and returns it as an integer.


@note The full 8-byte span must be valid; otherwise `Bad Address at <address>` is raised.

---

### How to read it

- Loads 8 bytes, **little-endian**.
- Unaligned 64-bit reads may **tear** if another agent is concurrently writing.

---

### Examples
```basic
REM Read a 64-bit value
Q = PEEKQ(&100080)
PRINT Q
```

---

### Notes
- Use for 64-bit fields or counters; don’t assume atomicity for unaligned addresses.

**See also:**  
\ref PEEK "PEEK" · \ref PEEKD "PEEKD" · \ref POKEQ "POKEQ"