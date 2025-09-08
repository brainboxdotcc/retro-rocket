\page CIRCLE CIRCLE Keyword
```basic
CIRCLE integer-expression, integer-expression, integer-expression, boolean-expression
```

Draws a circle using the current graphics colour (set with \ref GCOL "GCOL").

- First two parameters: the **X,Y coordinates** of the circle’s centre.  
- Third parameter: the **radius** in pixels.  
- Fourth parameter: a boolean; if `FALSE` only the outline is drawn, if `TRUE` the circle is filled.

![Circle outline example](https://github.com/user-attachments/assets/7338255b-bfed-4408-b8be-8cbb943e2198)

---

##### Example: Outline

```basic
GCOL RGB(255,0,0)
CIRCLE 400, 300, 100, FALSE
```

Draws a red circle outline centred at `(400,300)` with radius `100`.

---

##### Example: Filled

```basic
GCOL RGB(0,255,0)
CIRCLE 200, 200, 50, TRUE
```

Draws a filled green circle centred at `(200,200)` with radius `50`.

---

##### Notes
- Coordinates are in **screen pixels**; `(0,0)` is the top-left corner.  
- Circles are clipped at the screen edges if they extend beyond the display.  
- \ref GCOL "GCOL" accepts a 24-bit RGB colour, usually constructed with \ref RGB "RGB".

**See also:**
\ref GCOL "GCOL", \ref LINE "LINE", \ref RECTANGLE "RECTANGLE"
