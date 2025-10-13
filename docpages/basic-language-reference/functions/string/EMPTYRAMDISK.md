\page EMPTYRAMDISK EMPTYRAMDISK$ Function

```basic
EMPTYRAMDISK$(size-megabytes)
```

Creates a **RAM-backed block device and formats it as RetroFS**.
The parameter is the size of the new device in megabytes. Returns the device name on success.

---

### Examples

```basic
REM Create a 1GB RetroFS ramdisk
RD$ = EMPTYRAMDISK$(1024)
MOUNT "/ramdisk", RD$, "rfs"
PRINT "Created RAM disk"
```

---

### Notes

* The ram disk block size is always 512
* Device names are automatically assigned by the system (e.g. `"ram0"`, `"ram1"`).
* Contents are volatile and cleared when the system shuts down or discards the device.

---

**See also:**
\ref MOUNT "MOUNT"
