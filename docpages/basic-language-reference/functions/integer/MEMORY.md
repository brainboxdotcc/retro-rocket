\page MEMORY MEMORY Function

```basic
MEMORY
```

Returns the total **system memory** in bytes, including both **used** and **free** portions, for BASIC programs and the operating system.

---

### Examples

```basic
REM Print total system memory
PRINT MEMORY
```

On a 4 GB machine this will typically print:

```
4294967296
```

```basic
REM Derive free percentage
PRINT "Free % = "; (MEMFREE * 100) / MEMORY
```

If `MEMFREE` reports `3758096384` on a 4 GB machine, this prints:

```
Free % = 87
```

---

### Notes

* The value is in **bytes**, returned as a 64-bit integer.
* Reflects the **installed physical RAM** available to Retro Rocket OS.
* Together with \ref MEMFREE "MEMFREE" and \ref MEMUSED "MEMUSED", allows monitoring of system usage.
* On machines marketed as “4 GB”, the total may be slightly less due to reserved space for hardware.

---

\image html memory.png

---

**See also:**
\ref MEMFREE "MEMFREE" · \ref MEMUSED "MEMUSED" · \ref MEMORY "MEMRELEASE" · \ref MEMORY "MEMALLOC"
