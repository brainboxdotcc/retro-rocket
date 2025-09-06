\page LINE LINE Keyword
```basic
LINE integer-expression, integer-expression, integer-expression, integer-expression
```

Draws a straight line from the first **X,Y** pair to the second **X,Y** pair using the current graphics colour (set by [`GCOL`](https://github.com/brainboxdotcc/retro-rocket/wiki/GCOL)).

- Parameters are: `x1, y1, x2, y2`.

---

### Examples

Basic usage
```basic
GCOL RGB(255,0,0)
LINE 50, 50, 200, 120
```

Crosshair
```basic
GCOL RGB(0,255,0)
LINE 120, 0,   120, 240
LINE 0,   120, 240, 120
```

---


\remark If [`AUTOFLIP`](https://github.com/brainboxdotcc/retro-rocket/wiki/AUTOFLIP) is `FALSE`, draw your frame (including `LINE`) and then call [`FLIP`](https://github.com/brainboxdotcc/retro-rocket/wiki/FLIP) to present it.

---

### Notes
- Coordinates are in **screen pixels**; `(0,0)` is the **top-left** of the display.
- Drawing uses the **current graphics colour**; change it with `GCOL RGB(r,g,b)` before calling `LINE`.
- Lines extending beyond the screen are **clipped** at the display edge.

**See also:**  
[`GCOL`](https://github.com/brainboxdotcc/retro-rocket/wiki/GCOL) ·
[`RECTANGLE`](https://github.com/brainboxdotcc/retro-rocket/wiki/RECTANGLE) ·
[`CIRCLE`](https://github.com/brainboxdotcc/retro-rocket/wiki/CIRCLE) ·
[`PLOT`](https://github.com/brainboxdotcc/retro-rocket/wiki/PLOT) ·
[`FLIP`](https://github.com/brainboxdotcc/retro-rocket/wiki/FLIP) ·
[`AUTOFLIP`](https://github.com/brainboxdotcc/retro-rocket/wiki/AUTOFLIP)
