\page GCOL GCOL Keyword
```basic
GCOL integer-expression
```

Sets the **current graphics colour**. The colour value is given by `integer-expression`.

- Colours are represented as a **32-bit value** where each 8-bit segment corresponds to **alpha**, **red**, **green**, **blue** in that order.
- Use the [`RGB`](https://github.com/brainboxdotcc/retro-rocket/wiki/RGB) function to create a colour value from three 0–255 components.


\remark `GCOL` affects **graphics** drawing only. For **text** colours see [`COLOUR`](https://github.com/brainboxdotcc/retro-rocket/wiki/COLOUR) and [`BACKGROUND`](https://github.com/brainboxdotcc/retro-rocket/wiki/BACKGROUND).

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
- The **current graphics colour** is used by drawing statements such as [`LINE`](https://github.com/brainboxdotcc/retro-rocket/wiki/LINE), [`CIRCLE`](https://github.com/brainboxdotcc/retro-rocket/wiki/CIRCLE), [`RECTANGLE`](https://github.com/brainboxdotcc/retro-rocket/wiki/RECTANGLE), and [`PLOT`](https://github.com/brainboxdotcc/retro-rocket/wiki/PLOT).
- `GCOL` does not affect the text foreground or background; use `COLOUR` and `BACKGROUND` for text.

**See also:** [`RGB`](https://github.com/brainboxdotcc/retro-rocket/wiki/RGB), [`COLOUR`](https://github.com/brainboxdotcc/retro-rocket/wiki/COLOUR), [`BACKGROUND`](https://github.com/brainboxdotcc/retro-rocket/wiki/BACKGROUND)
