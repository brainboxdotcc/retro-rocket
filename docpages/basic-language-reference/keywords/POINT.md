\page POINT POINT Keyword
```basic
POINT integer-expression, integer-expression
```

Draws a single pixel at the **X,Y** coordinates given, using the current graphics colour (set with \ref GCOL "GCOL").

- Parameters are: `x, y` (screen pixels).

---

### Examples

Single white dot
```basic
GCOL RGB(255,255,255)
POINT 100, 80
```

Star field
```basic
GCOL RGB(200,200,255)
FOR I = 1 TO 200
    X = RND(1920)
    Y = RND(1080)
    POINT X, Y
NEXT
```

---

### Notes
- Coordinates are in **screen pixels**; `(0,0)` is the **top-left** corner.
- Points outside the screen are clipped.
- When \ref AUTOFLIP "AUTOFLIP" is `FALSE`, the pixel appears after you call \ref FLIP "FLIP".

**See also:**  
\ref GCOL "GCOL" ·
\ref LINE "LINE" ·
\ref RECTANGLE "RECTANGLE" ·
\ref CIRCLE "CIRCLE"
