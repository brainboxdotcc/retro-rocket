\page vols vols command

```basic
vols <>device-name>
```

Displays all volumes found on the specified device.

Each volume is shown in partition order.

\image html vols.png

---

**Notes**

* The device name must refer to a valid block device.

---

**Errors**

* `No device name specified` if no device name is given.

---

**Examples**

```basic
vols hd0
```
