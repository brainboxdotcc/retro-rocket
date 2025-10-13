\page RAMDISK RAMDISK$ Function

```basic
RAMDISK$(string-expression)
```

Creates a **RAM-backed block device** from the content of an existing block device.
The parameter is the name of the source device (e.g. `"hd0"`).
Returns the **name of the new RAM disk device** as a string.

---

### Examples

```basic
REM Duplicate an existing block device into RAM
ram$ = RAMDISK$("hd0")
PRINT "Created RAM disk: "; ram$
```

---

### Notes

* The new device is a complete in-memory copy of the original device.
* Useful for testing, running faster I/O, or working on a safe copy.
* RAM disks are volatile: contents are lost when the system is rebooted or the device is discarded.

**See also:**
\ref MOUNT "MOUNT"
