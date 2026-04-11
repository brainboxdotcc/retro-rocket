\page ADFSIMAGE ADFSIMAGE$ Function

```basic
ADFSIMAGE$(path$)
```

Creates a **RAM-backed block device from an ADFS disk image**.
The parameter is the path to an ADFS S/M/L image file. Returns the device name on success.

---

### Examples

```basic
REM Load an ADFS disk image into RAM
RD$ = ADFSIMAGE$("chukie-egg.adl")
MOUNT "/games", RD$, "adfs"
PRINT "ADFS image mounted"
```

---

### Notes

* The image must be a valid ADFS S/M/L disk image
* The image is loaded entirely into memory
* The ram disk block size is always 256
* Device names are automatically assigned by the system (e.g. `"ram0"`, `"ram1"`)
* The image buffer is owned by the ram disk after creation
* Contents are volatile and lost when the system shuts down or discards the device

---

**See also:**
\ref MOUNT "MOUNT"
