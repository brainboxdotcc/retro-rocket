\page TRIANGLE TRIANGLE Keyword
```basic
TRIANGLE integer-expression, integer-expression, integer-expression, integer-expression, integer-expression, integer-expression
```

Draws a **filled triangle** using the current graphics colour (set with \ref GCOL "GCOL").

- Parameters are: `x1, y1, x2, y2, x3, y3` - three vertex positions in screen pixels.
- The triangle is always **filled**. There is no outline-only mode.

---

### Examples

Filled triangle
```basic
GCOL RGB(255,0,0)
TRIANGLE 60, 30, 180, 50, 90, 160
```

Two triangles with different colours
```basic
GCOL RGB(0,150,255)
TRIANGLE 40, 40, 120, 40, 80, 120

GCOL RGB(0,200,80)
TRIANGLE 140, 60, 220, 100, 180, 160
```

Outline effect using `LINE`
```basic
X1 = 60 : Y1 = 30
X2 = 180 : Y2 = 50
X3 = 90 : Y3 = 160

GCOL RGB(255,255,0)
TRIANGLE X1, Y1, X2, Y2, X3, Y3

GCOL RGB(0,0,0)
LINE X1, Y1, X2, Y2
LINE X2, Y2, X3, Y3
LINE X3, Y3, X1, Y1
```

---

### Notes
- Coordinates are in **screen pixels**; `(0,0)` is the **top-left** corner.
- Triangles that extend beyond the screen are **clipped** at the display edge.
- Change the drawing colour with `GCOL RGB(r,g,b)` before calling `TRIANGLE`.
- When \ref AUTOFLIP "AUTOFLIP" is `FALSE`, drawn frames appear only after \ref FLIP "FLIP".

**See also:**  
\ref GCOL "GCOL" ·
\ref LINE "LINE" ·
\ref RECTANGLE "RECTANGLE" ·
\ref CIRCLE "CIRCLE"
