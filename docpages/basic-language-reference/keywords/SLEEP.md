\page SLEEP SLEEP Keyword

```basic
SLEEP expression
```

Suspends execution of the current program for the specified number of milliseconds.
The expression must evaluate to a positive integer. Fractions are ignored; resolution is to the nearest whole millisecond only.

During a `SLEEP` the process is suspended and does not consume CPU time. Other programs continue to run.


\remark You can cancel a long sleep with `CTRL+ESC`. Without an error handler the
@note programme terminates; with one, control passes to `ON ERROR`.

---

#### Example

```basic
PRINT "START"
SLEEP 2000
PRINT "AFTER 2 SECONDS"
```

Output:

```
START
AFTER 2 SECONDS
```

---

#### Notes

* Only whole milliseconds are supported. `SLEEP 2.5` is treated as `SLEEP 2`.
* A `SLEEP 0` has no effect.
* Long sleeps (hundreds or thousands of seconds) are possible, but the process will remain suspended until the time has passed.