\page PAGE PAGE Keyword

```basic
PAGE ON
PAGE OFF
```

Enable or disable paged output. When enabled, the system pauses after each full screen of text and waits for a keypress before continuing.

---

* **PAGE ON**

  * Enables paged mode (equivalent to `VDU 14`).
  * Output pauses after each full screen.
  * A prompt is shown on the last line.

* **PAGE OFF**

  * Disables paged mode (equivalent to `VDU 15`).
  * Output continues without interruption.

---

##### Example

```basic
PAGE ON

FOR I = 1 TO 100
    PRINT "Line "; I
NEXT

PAGE OFF
PRINT "No more pauses"
```

---

##### Notes

* Paged mode affects all subsequent text output until disabled.
* Any keypress resumes output when paused.
* `PAGE` only affects console output; it does not alter program flow.
