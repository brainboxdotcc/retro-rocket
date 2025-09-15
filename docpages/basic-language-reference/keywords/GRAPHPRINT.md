\page GRAPHPRINT GRAPHPRINT Keyword

```basic
GRAPHPRINT x, y, printable
```

Draws text directly onto the graphics framebuffer at **pixel coordinates**.
Unlike `PRINT`, `GRAPHPRINT` does not use or update the text cursor. Each call is independent.

---

### Notes

* `(x, y)` are **pixel coordinates** measured from the top-left corner of the screen.
* `printable` is the same syntax accepted by `PRINT`: a list of values separated by commas or semicolons.

  * **Strings** are rendered literally.
  * **Numeric values** are converted to their string form.
  * `;` continues text without spacing.
  * `,` advances to the next tab stop (multiple of text column width).
* Text is rendered using the current **graphics colour** (`GCOL`) for the foreground.
* The background is always **transparent**; existing pixels are left unchanged.
* The function clips safely against screen edges; partially visible text is drawn correctly.
* All 8-bit codes `0x00–0xFF` map directly to glyphs, including those redefined with `VDU 23`. No UTF-8 decoding is applied.
* Each call draws at the specified location only. There is no graphics cursor or scrolling.

---

### Errors

* An error occurs if `x` or `y` are not numeric.
* An error occurs if `printable` is omitted.

---

### Examples

**Draw a label in magenta**

```basic
GCOL &FF00FF
GRAPHPRINT 100, 40, "SCORE:", SCORE
```

**Overlay transparent HUD text**

```basic
GCOL &FFFFFF
GRAPHPRINT 8, 8, "READY!";
```

**Use a redefined glyph**

```basic
VDU 23,65,&3C,&7E,&FF,&E7,&FF,&66,&66,&00
GRAPHPRINT 120, 100, "AAAAAAAAAAA"
```

---

### See also

* \ref PRINT "PRINT" - text output in the scrolling text grid.
* \ref GCOL "GCOL" - set graphics colour (0xRetro RocketGGBB).
* \ref LINE "LINE" - draw lines (used by `VDU 25` in Retro Rocket).
* \ref VDU "VDU" 23 - redefine character bitmaps used by both `PRINT` and `GRAPHPRINT`.