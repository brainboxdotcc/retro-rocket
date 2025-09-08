\page FLIP FLIP Keyword
```basic
FLIP
```

When \ref AUTOFLIP "AUTOFLIP" is set to `FALSE`, `FLIP` **presents the current backbuffer** to the screen (frontbuffer).
This gives you explicit control over when a frame becomes visible - useful for animations or multi-step drawing.

- Drawing statements (e.g. \ref LINE "LINE", \ref CIRCLE "CIRCLE", \ref RECTANGLE "RECTANGLE", \ref PLOT "PLOT") update the **backbuffer**.
- `FLIP` swaps the backbuffer to the display in one step, so the whole frame appears at once.
- If `AUTOFLIP` is `TRUE`, the system presents frames automatically and you typically do not call `FLIP` yourself.

---

##### Example: manual presentation

```basic
AUTOFLIP FALSE

X = 40
GCOL RGB(0,255,0)

WHILE X < 300
    CLS
    CIRCLE X, 120, 20, TRUE
    FLIP
    X = X + 5
ENDWHILE
```

This draws a moving filled circle and shows each frame explicitly with `FLIP`.

---

##### Notes
- If you do not call `FLIP` while `AUTOFLIP` is `FALSE`, the screen will not update until the next flip.
- `CLS` clears both text and graphics; use it when you want a fresh frame for the next draw.
- Colour for graphics is set with \ref GCOL "GCOL", typically via `RGB(r,g,b)`.

**See also:**  
\ref AUTOFLIP "AUTOFLIP" ·
\ref GCOL "GCOL" ·
\ref CIRCLE "CIRCLE" ·
\ref LINE "LINE" ·
\ref RECTANGLE "RECTANGLE" ·
\ref PLOT "PLOT"
