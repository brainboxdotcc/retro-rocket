\page PLOT PLOT Keyword
```basic
PLOT integer-variable, integer-expression, integer-expression
```

Draws a previously loaded **sprite** at the given **X,Y** position.  
The first parameter must be an **integer variable** holding the **sprite handle** returned by [`SPRITELOAD`](https://github.com/brainboxdotcc/retro-rocket/wiki/SPRITELOAD).  
The sprite is positioned with its **top-left corner** at `(X, Y)` in screen pixels.


\remark The first argument must be a **variable** containing a valid sprite handle, not a literal value or expression.


\remark Use the built-in `GRAPHICS_WIDTH` and `GRAPHICS_HEIGHT` to place sprites relative to the current screen size.

---

### Example

```basic
SPRITELOAD brain,"/images/brainbox.png"
SPRITELOAD background,"/images/computer.jpg"

PLOT background,0,0
PLOT brain,0,0
PLOT brain,GRAPHICS_WIDTH-250,0
PLOT brain,0,GRAPHICS_HEIGHT-250
PLOT brain,1920-250,GRAPHICS_HEIGHT-250
```

![image](https://github.com/brainboxdotcc/retro-rocket/assets/1556794/63be727a-9495-4a14-b6b5-c5265fc694c2)

---

### Notes
- Coordinates are in **screen pixels**; `(0,0)` is the **top-left** of the display.
- Sprites that extend beyond the screen are **clipped** at the edges.
- When [`AUTOFLIP`](https://github.com/brainboxdotcc/retro-rocket/wiki/AUTOFLIP) is `FALSE`, drawn frames become visible only after [`FLIP`](https://github.com/brainboxdotcc/retro-rocket/wiki/FLIP).
- Free sprite resources when no longer needed with [`SPRITEFREE`](https://github.com/brainboxdotcc/retro-rocket/wiki/SPRITEFREE).

**See also:**  
[`SPRITELOAD`](https://github.com/brainboxdotcc/retro-rocket/wiki/SPRITELOAD) ·
[`SPRITEFREE`](https://github.com/brainboxdotcc/retro-rocket/wiki/SPRITEFREE) ·
[`PLOTQUAD`](https://github.com/brainboxdotcc/retro-rocket/wiki/PLOTQUAD) ·
[`AUTOFLIP`](https://github.com/brainboxdotcc/retro-rocket/wiki/AUTOFLIP) ·
[`FLIP`](https://github.com/brainboxdotcc/retro-rocket/wiki/FLIP)
