\page CURRENTX CURRENTX Function

```basic
CURRENTX
```

Returns the current **X text cursor position** (column) on the terminal, measured in **character cells**.
The leftmost column is **0**, increasing to the right.

---

### Examples

```basic
CLS
PRINT "Hello";
x = CURRENTX
PRINT " (cursor now at column "; x; ")"
```

```basic
REM Draw a horizontal line of dashes across the current row
CLS
FOR i = 1 TO TERMWIDTH
    PRINT "-";
NEXT
PRINT
PRINT "Line ended at column "; CURRENTX
```

```basic
REM Example: align text to screen centre using CURRENTX
CLS
PRINT "Retro Rocket";
middle = TERMWIDTH / 2
shift = middle - CURRENTX / 2
CURSOR shift, 5
PRINT "Centered beneath"
```

---

### Notes

* Cursor position is **zero-based**: the first column is `0`.
* `CURRENTX` only reflects the **logical cursor position**, not the underlying ANSI escape codes used by `PRINT`.
* When the cursor advances past the last column, it wraps onto the next row. At this point `CURRENTX` is reset to `0`.
* Use together with \ref CURRENTY "CURRENTY" to track both horizontal and vertical positions.

---

**See also:**
\ref CURRENTY "CURRENTY" · \ref CURSOR "CURSOR" · \ref TERMWIDTH "TERMWIDTH"
