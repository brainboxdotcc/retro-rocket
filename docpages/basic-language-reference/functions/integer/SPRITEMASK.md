\page SPRITEMASK SPRITEMASK Function

```basic
SPRITEMASK(sprite-handle, x, y)
```

Returns the **mask value** of the pixel at the given coordinates within a sprite.

The mask is typically used to determine whether a pixel is visible or transparent. A non-zero value usually means the pixel is drawn, while zero means it is masked (not drawn).

The top-left pixel of the sprite is `(0, 0)`. The `x` value moves across from left to right, and the `y` value moves down from top to bottom.

If the sprite handle is invalid, or the coordinates are outside the sprite's dimensions, an error is raised.

---

### Examples

```basic
m = SPRITEMASK(player_sprite, 10, 5)
PRINT "Mask value: "; m
```

```basic
IF SPRITEMASK(enemy_sprite, x, y) = 0 THEN
    PRINT "Pixel is transparent"
ENDIF
```

**See also:**
\ref SPRITEPIXEL "SPRITEPIXEL"
