\page RECTANGLE RECTANGLE Keyword
```basic
RECTANGLE integer-expression, integer-expression, integer-expression, integer-expression
```

Draws a **filled**, axis-aligned rectangle using the current graphics colour (set with [`GCOL`](https://github.com/brainboxdotcc/retro-rocket/wiki/GCOL)).

- Parameters are: `x1, y1, x2, y2` - the two opposite corners of the rectangle in screen pixels.
- The rectangle is filled; there is no outline-only form.

---

### Examples

Filled rectangle
```basic
GCOL RGB(0,128,255)
RECTANGLE 50, 50, 180, 120
```

Two rectangles with different colours
```basic
GCOL RGB(255,0,0)
RECTANGLE 20, 20, 100, 60

GCOL RGB(0,200,0)
RECTANGLE 120, 30, 220, 110
```

Simple progress bar
```basic
REM Background bar
GCOL RGB(60,60,60)
RECTANGLE 40, 200, 440, 230

REM Progress amount
GCOL RGB(0,180,0)
RECTANGLE 40, 200, 40 + 300, 230
```

---

### Notes
- Coordinates are in **screen pixels**; `(0,0)` is the **top-left** corner.
- Rectangles that extend beyond the screen are **clipped** at the edges.
- Change the drawing colour with `GCOL RGB(r,g,b)` before calling `RECTANGLE`.
- When [`AUTOFLIP`](https://github.com/brainboxdotcc/retro-rocket/wiki/AUTOFLIP) is `FALSE`, drawn frames appear only after [`FLIP`](https://github.com/brainboxdotcc/retro-rocket/wiki/FLIP).

**See also:**  
[`GCOL`](https://github.com/brainboxdotcc/retro-rocket/wiki/GCOL) ·
[`LINE`](https://github.com/brainboxdotcc/retro-rocket/wiki/LINE) ·
[`CIRCLE`](https://github.com/brainboxdotcc/retro-rocket/wiki/CIRCLE) ·
[`POINT`](https://github.com/brainboxdotcc/retro-rocket/wiki/POINT) ·
[`PLOT`](https://github.com/brainboxdotcc/retro-rocket/wiki/PLOT)
