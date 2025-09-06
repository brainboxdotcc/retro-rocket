\page CURSOR CURSOR Keyword
```basic
CURSOR integer-expression, integer-expression
```

Moves the **text cursor** to the given `X,Y` coordinates (column, row).  
The origin is the **top-left corner** at `(0,0)`.

- `X` = column (0 is the leftmost column)
- `Y` = row    (0 is the top row)

After moving the cursor, subsequent [`PRINT`](https://github.com/brainboxdotcc/retro-rocket/wiki/PRINT) output starts at that position.

---

##### Example

```basic
CLS
CURSOR 10, 5
PRINT "Hello from (10,5)"
```

---


> Use [`TERMWIDTH`](https://github.com/brainboxdotcc/retro-rocket/wiki/TERMWIDTH) and
> [`TERMHEIGHT`](https://github.com/brainboxdotcc/retro-rocket/wiki/TERMHEIGHT) to compute
> safe positions or to centre text.

---

##### Notes
- Affects the **text layer** only; graphics primitives (e.g. [`LINE`](https://github.com/brainboxdotcc/retro-rocket/wiki/LINE)) are unaffected.
- The cursor is reset to `(0,0)` by [`CLS`](https://github.com/brainboxdotcc/retro-rocket/wiki/CLS).
- Valid screen coordinates depend on the current terminal size (`0..TERMWIDTH-1`, `0..TERMHEIGHT-1`).
