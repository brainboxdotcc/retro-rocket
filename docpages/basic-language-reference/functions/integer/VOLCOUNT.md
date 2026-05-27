\page VOLCOUNT VOLCOUNT Function

```basic
numeric-value = VOLCOUNT(device-name$)
```

Returns the number of volumes found on the specified device.

---

**Notes**

* The device name must refer to a valid block device.
* Volumes are counted in partition order.
* If no volumes are found, `0` is returned.

---

**Examples**

```basic
PRINT VOLCOUNT("hd0")
```

