\page SPRITELOAD SPRITELOAD Keyword
```basic
SPRITELOAD integer-variable, string-expression
```

Loads an image from disk as a **sprite** and stores its **handle** in the given integer variable.  
Use the handle with [`PLOT`](https://github.com/brainboxdotcc/retro-rocket/wiki/PLOT) or [`PLOTQUAD`](https://github.com/brainboxdotcc/retro-rocket/wiki/PLOTQUAD), and free it with [`SPRITEFREE`](https://github.com/brainboxdotcc/retro-rocket/wiki/SPRITEFREE) when finished.


> The **first argument must be an integer variable**, not a literal or expression.  
> Paths are **case-insensitive** and `.` / `..` are **not supported** in paths.

---

### Supported formats

| Extension | File type                  |
|----------:|----------------------------|
| `JPG`     | JPEG                       |
| `PNG`     | Portable Network Graphics  |
| `TGA`     | TARGA                      |
| `BMP`     | Windows DIB Bitmap         |
| `PSD`     | Photoshop Document         |
| `GIF`     | Graphics Interchange Format|
| `PIC`     | Pictor Paint (PC Paint)    |


> Each program may have **up to 1024 sprites** loaded at once.  
> Handles are **per-program**. All sprites are **auto-freed** when the program ends, but it’s good practice to call `SPRITEFREE` yourself when you no longer need a sprite.

---

### Examples

**Load and draw a sprite**
```basic
SPRITELOAD S, "/images/logo.png"
PLOT S, 100, 80
SPRITEFREE S
```

**Background plus multiple placements (using screen size)**
```basic
SPRITELOAD BG, "/images/computer.jpg"
SPRITELOAD BRAIN, "/images/brainbox.png"

PLOT BG, 0, 0
PLOT BRAIN, 0, 0
PLOT BRAIN, GRAPHICS_WIDTH - 250, 0
PLOT BRAIN, 0, GRAPHICS_HEIGHT - 250
PLOT BRAIN, GRAPHICS_WIDTH - 250, GRAPHICS_HEIGHT - 250
```

**Project a sprite onto a quad**
```basic
SPRITELOAD S, "/images/tile.png"
PLOTQUAD S,  60,180, 260,180, 220,120, 100,110
SPRITEFREE S
```

**Handle load errors**
```basic
ON ERROR PROCload_fail
SPRITELOAD S, "/images/missing.png"
PRINT "Loaded";  REM only reached if no error
END

DEF PROCload_fail
    PRINT "Sprite load failed: "; ERR$
ENDPROC
```

---

### Behaviour and usage notes
- `string-expression` is the **filename or path** to the image. Use `CHDIR` to set the working directory or supply an absolute path starting with `/`.
- Drawing respects the current graphics state (for example, `AUTOFLIP` and `FLIP`). If `AUTOFLIP` is `FALSE`, your drawing appears when you call `FLIP`.
- Free sprite resources explicitly with `SPRITEFREE` to reclaim memory during long-running programs.

**See also:**  
[`PLOT`](https://github.com/brainboxdotcc/retro-rocket/wiki/PLOT) ·
[`PLOTQUAD`](https://github.com/brainboxdotcc/retro-rocket/wiki/PLOTQUAD) ·
[`SPRITEFREE`](https://github.com/brainboxdotcc/retro-rocket/wiki/SPRITEFREE) ·
[`CHDIR`](https://github.com/brainboxdotcc/retro-rocket/wiki/CHDIR) ·
[`AUTOFLIP`](https://github.com/brainboxdotcc/retro-rocket/wiki/AUTOFLIP) ·
[`FLIP`](https://github.com/brainboxdotcc/retro-rocket/wiki/FLIP)
