\page GCOL GCOL Keyword
```basic
GCOL integer-expression
```

Sets the **current graphics colour**. The colour value is given by `integer-expression`.

- Colours are represented as a **32-bit value** where each 8-bit segment corresponds to **alpha**, **red**, **green**, **blue** in that order.
- Use the \ref RGB "RGB" function to create a colour value from three 0–255 components.


\remark `GCOL` affects **graphics** drawing only. For **text** colours see \ref COLOUR "COLOUR" and \ref BACKGROUND "BACKGROUND".

---

### Examples

Set a graphics colour then draw an outline circle
```basic
GCOL RGB(255,0,0)
CIRCLE 200, 150, 80, FALSE
```

Set a graphics colour then draw a filled rectangle
```basic
GCOL RGB(0,128,255)
RECTANGLE 50, 50, 120, 90
```

---

### Notes
- Each colour component is an integer in the range **0–255**.
- The **current graphics colour** is used by drawing statements such as \ref LINE "LINE", \ref CIRCLE "CIRCLE", \ref RECTANGLE "RECTANGLE", and \ref PLOT "PLOT".
- `GCOL` does not affect the text foreground or background; use `COLOUR` and `BACKGROUND` for text.

**See also:**
\ref RGB "RGB", \ref COLOUR "COLOUR", \ref BACKGROUND "BACKGROUND"
