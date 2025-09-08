\page CURSOR CURSOR Keyword
```basic
CURSOR integer-expression, integer-expression
```

Moves the **text cursor** to the given `X,Y` coordinates (column, row).  
The origin is the **top-left corner** at `(0,0)`.

- `X` = column (0 is the leftmost column)
- `Y` = row    (0 is the top row)

After moving the cursor, subsequent \ref PRINT "PRINT" output starts at that position.

---

##### Example

```basic
CLS
CURSOR 10, 5
PRINT "Hello from (10,5)"
```

---


@note Use \ref TERMWIDTH "TERMWIDTH" and
@note \ref TERMHEIGHT "TERMHEIGHT" to compute
@note safe positions or to centre text.

---

##### Notes
- Affects the **text layer** only; graphics primitives (e.g. \ref LINE "LINE") are unaffected.
- The cursor is reset to `(0,0)` by \ref CLS "CLS".
- Valid screen coordinates depend on the current terminal size (`0..TERMWIDTH-1`, `0..TERMHEIGHT-1`).
