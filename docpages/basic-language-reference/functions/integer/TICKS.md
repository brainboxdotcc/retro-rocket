\page TICKS TICKS Function

```basic
TICKS
```

Returns the system **uptime in milliseconds**.

The value represents how long the system has been running since it was started, with finer precision than \ref UPSECS "UPSECS".

---

### Examples

```basic
REM Print uptime in milliseconds
PRINT "Uptime (ms): "; TICKS
```

```basic
REM Measure how long something takes
START = TICKS
PROCdostuff() ' ... do something ...
ELAPSED = TICKS - START
PRINT "Elapsed time: "; ELAPSED; " ms"
```

```basic
REM Simple delay loop
START = TICKS
WHILE TICKS - START < 1000
ENDWHILE
PRINT "1 second has passed"
```

---

### Notes

* Returns a steadily increasing integer value starting from **0** at system boot.
* Resolution is **milliseconds**, suitable for more precise timing than seconds.
* Ideal for profiling, animation timing, and short delays.

---

**See also:**
\ref UPSECS "UPSECS" · \ref SLEEP "SLEEP"
