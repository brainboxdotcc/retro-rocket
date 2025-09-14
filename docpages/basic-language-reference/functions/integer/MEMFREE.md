\page MEMFREE MEMFREE Function

```basic
MEMFREE
```

Returns the total amount of **free memory** available to BASIC programs and the operating system, in **bytes**.

---

### Examples

```basic
REM Print free memory
PRINT MEMFREE
```

On a 4 GB machine with \~3.5 GB available, this might print:

```
3758096384
```

```basic
REM Convert to megabytes for easier reading
PRINT MEMFREE / (1024 * 1024); " MB free"
```

On the same 4 GB machine, this might print:

```
3584 MB free
```

```basic
REM Simple check for low memory
IF MEMFREE < (512 * 1024 * 1024) THEN
    PRINT "Warning: less than 512 MB free"
ENDIF
```

---

### Notes

* The value is in **bytes**, returned as a 64-bit integer.
* Represents free space across the whole system (OS + BASIC).
* Actual available memory on a “4 GB” system is typically less than 4096 MB due to hardware reservations.
* The value changes dynamically as programs allocate or free memory.

---

**See also:**
\ref MEMUSED "MEMUSED" · \ref MEMORY "MEMORY" · \ref MEMORY "MEMRELEASE" · \ref MEMORY "MEMALLOC"
