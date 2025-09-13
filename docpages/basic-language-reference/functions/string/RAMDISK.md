\page RAMDISK RAMDISK$ Function

## Creating a ramdisk from another block device

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

---

## Creating blank ramdisks

```basic
RAMDISK(integer-expression, integer-expression)
```

Creates a **new RAM-backed block device** with the given dimensions.

* First parameter = number of blocks.
* Second parameter = block size in bytes.

The device is initially **unformatted** and cannot be mounted until a filesystem is created on it.
Returns the **name of the new RAM disk device** as a string.

---

### Examples

```basic
REM Create a 10 MB RAM disk with 512-byte blocks
ram$ = RAMDISK(20480, 512)
PRINT "Created RAM disk: "; ram$
```

---

### Notes

* Size = `blocks × blocksize`.
* Device names are automatically assigned by the system (e.g. `"ram0"`, `"ram1"`).
* Contents are volatile and cleared when the system shuts down or discards the device.

---

**See also:**
\ref MOUNT "MOUNT"
