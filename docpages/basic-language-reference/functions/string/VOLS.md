\page VOL VOL$ Function

```basic
string-value = VOL$(device-name$, volume-index)
```

Returns the description of a volume on the specified device.

Volumes are enumerated in partition order starting from index `0`.

If the specified volume does not exist, an empty string is returned.

---

**Notes**

* The device name must refer to a valid block device.
* Volume descriptions are filesystem dependent.
* Invalid indexes return an empty string.

---

**Examples**

```basic
PRINT VOL$("hd0", 0)
PRINT VOL$("hd0", 1)
```
