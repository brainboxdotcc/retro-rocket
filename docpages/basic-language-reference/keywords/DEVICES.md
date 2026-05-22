\page DEVICES DEVICES Statement

```basic
DEVICES string-devices$, string-owners$, integer-count
```

Writes the currently registered device names and their owning subsystems into two string arrays.

`string-devices$` receives full device names such as `hd0`, `cd0`, `ram0`, and `net0`. `string-owners$` receives the subsystem which registered each name, such as `ahci`, `nvme`, or `ramdisk`. `integer-count` is set to the number of entries written.

---

### Examples

```basic
DEVICES d$, o$, count

FOR i = 0 TO count - 1
    PRINT d$(i), o$(i)
NEXT
```

---

### Notes

The arrays are created or resized automatically. The two arrays always have matching indexes, so `d$(0)` and `o$(0)` describe the same device.

Device names are kernel namespace names, not files or paths.

---

**See also:**
\ref MOUNT "MOUNT" · \ref MODLOAD "MODLOAD"
