\page ANIMATE ANIMATE Keyword

```basic
ANIMATE NEXT integer-variable
ANIMATE RESET integer-variable
ANIMATE ON integer-variable
ANIMATE OFF integer-variable
```

Advances or controls playback of an **animated sprite** (such as a GIF loaded with \ref SPRITELOAD "SPRITELOAD").
The first parameter must be an **integer variable** holding the **sprite handle** returned by \ref SPRITELOAD "SPRITELOAD".

* `ANIMATE NEXT`
  Moves the sprite forward to its next frame. If the sprite has reached the final frame, behaviour depends on its loop setting (see below).

* `ANIMATE RESET`
  Rewinds the sprite back to its first frame.

* `ANIMATE ON`
  Enables looping. After the final frame, playback continues from the first frame.

* `ANIMATE OFF`
  Disables looping. After the final frame, playback stops on that frame.

\remark The first argument must be a **variable** containing a valid sprite handle, not a literal value or expression.

---

### Example

```basic
PRINT "Loading... ";
SPRITELOAD bad_apple, "/images/bad-apple.gif"
CLS
AUTOFLIP FALSE

GCOL &888888
RECTANGLE 0, 0, GRAPHICS_WIDTH - 1, GRAPHICS_HEIGHT - 1

ANIMATE ON bad_apple

REPEAT
    PLOT bad_apple, GRAPHICS_CENTRE_X - 160, GRAPHICS_CENTRE_Y - 100
    FLIP
    SLEEP 66
    ANIMATE NEXT bad_apple
UNTIL INKEY$ <> ""

AUTOFLIP TRUE
CLS
END
```

---

### Notes

* If the sprite is not animated (single-frame image), `ANIMATE` statements are silent no-ops.
* Looping (`ANIMATE ON`) is enabled by default when a sprite is loaded.
* Frame delays stored in GIF metadata are currently ignored; use `SLEEP` or your own timing logic for consistent playback speed.
* To free sprite resources when no longer needed, use \ref SPRITEFREE "SPRITEFREE".

**See also:**
\ref SPRITELOAD "SPRITELOAD" ·
\ref SPRITEFREE "SPRITEFREE" ·
\ref PLOT "PLOT" ·
\ref AUTOFLIP "AUTOFLIP" ·
\ref FLIP "FLIP"
