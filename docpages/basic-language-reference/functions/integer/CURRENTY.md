\page CURRENTY CURRENTY Function

```basic
CURRENTY
```

Returns the current **Y text cursor position** (row) on the terminal, measured in **character cells**.
The topmost row is **0**, increasing downwards.

---

### Examples

```basic
CLS
PRINT "Hello"
y = CURRENTY
PRINT " (cursor now at row "; y; ")"
```

```basic
REM Demonstrate cursor movement
CLS
CURSOR 10, 5
PRINT "Placed at (10,5)"
PRINT "Now Y = "; CURRENTY
```

```basic
REM Use CURRENTY with looping to draw diagonals
CLS
FOR i = 0 TO 9
    CURSOR i, CURRENTY + 1
    PRINT "*";
NEXT
```

---

### Notes

* Cursor position is **zero-based**: the top row is `0`.
* `CURRENTY` reflects the current logical row, not raw terminal escape codes.
* When the cursor advances beyond the last row, the terminal scrolls and `CURRENTY` remains clamped to the bottom row.
* Use together with \ref CURRENTX "CURRENTX" to get the full cursor coordinates.

---

**See also:**
\ref CURRENTX "CURRENTX" · \ref CURSOR "CURSOR" · \ref TERMHEIGHT "TERMHEIGHT"
