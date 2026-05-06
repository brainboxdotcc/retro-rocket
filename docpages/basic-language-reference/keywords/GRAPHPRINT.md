\page GRAPHPRINT GRAPHPRINT Keyword

```basic
GRAPHPRINT x, y, scale_x, scale_y, printable
```

Draws text directly onto the graphics framebuffer at **pixel coordinates**.
Unlike `PRINT`, `GRAPHPRINT` does not use or update the text cursor. Each call is independent.

`x` and `y` are integer **pixel coordinates** measured from the top-left corner of the screen. scale_x and scale_y can be an integer or a real number. If you specify 0 for either dimension, the default scale of 1 will be used.
`printable` is the same syntax accepted by \ref PRINT "PRINT", a list of values separated by commas or semicolons. * Text is rendered using the current **graphics colour** (`GCOL`) for the foreground.
The background is always **transparent**, existing pixels are left unchanged. Each call draws at the specified location only, there is no graphics cursor or scrolling.

You can use negative scale values to flip the text. Note that if you flip the scale values, it will render right to left or bottom to top accordingly.

### Examples

**Draw a label upside down in magenta**

```basic
GCOL &FF00FF
GRAPHPRINT 100, 40, 1, -1, "SCORE:", SCORE
```

**Overlay transparent HUD text at 5x scale**

```basic
GCOL &FFFFFF
GRAPHPRINT 8, 8, 5, 5, "READY!";
```

**Use a redefined glyph**

```basic
VDU 23,65,&3C,&7E,&FF,&E7,&FF,&66,&66,&00
GRAPHPRINT 120, 100, 10, 10, "AAA"
```

### See also

* \ref PRINT "PRINT" ·
* \ref GCOL "GCOL" ·
* \ref LINE "LINE" ·
* \ref VDU "VDU 23"