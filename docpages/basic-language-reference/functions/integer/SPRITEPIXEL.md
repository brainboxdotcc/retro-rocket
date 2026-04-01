\page SPRITEPIXEL SPRITEPIXEL Function

```basic
SPRITEPIXEL(sprite-handle, x, y)
```

Returns the **colour value** of the pixel at the given coordinates within a sprite.

The top-left pixel of the sprite is `(0, 0)`. The `x` value moves across from left to right, and the `y` value moves down from top to bottom.

If the sprite handle is invalid, or the coordinates are outside the sprite's dimensions, an error is raised.

---

### Examples

```basic
c = SPRITEPIXEL(player_sprite, 10, 5)
PRINT "Pixel colour: "; c
```

```basic
IF SPRITEPIXEL(enemy_sprite, 0, 0) = 0 THEN
    PRINT "Top-left pixel is transparent or black"
ENDIF
```

**See also:**
\ref SPRITEWIDTH "SPRITEWIDTH", \ref SPRITEHEIGHT "SPRITEHEIGHT"
