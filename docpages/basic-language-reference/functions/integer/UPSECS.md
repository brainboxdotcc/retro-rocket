\page UPSECS UPSECS Function

```basic
UPSECS
```

Returns the system **uptime in seconds**.

The value represents how long the system has been running since it was started.

---

### Examples

```basic
REM Print uptime in seconds
PRINT "Uptime (seconds): "; UPSECS
```

```basic
REM Convert uptime to minutes and seconds
SECONDS = UPSECS
PRINT "Uptime: "; SECONDS / 60; " minutes"
```

---

### Notes

* Returns a steadily increasing integer value starting from **0** at system boot.
* Resolution is **seconds**; sub-second precision is not provided.
* Suitable for simple timing, delays, and measuring elapsed time.

---

**See also:**
\ref TIME "TIME" · \ref SLEEP "SLEEP"
