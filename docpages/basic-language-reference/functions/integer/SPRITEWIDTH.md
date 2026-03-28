\page SPRITEWIDTH SPRITEWIDTH Function

```basic
SPRITEWIDTH(sprite-handle)
```

Returns the **width** of a sprite in pixels.

---

### Examples

```basic
w = SPRITEWIDTH(player_sprite)
PRINT "Width: "; w
```

```basic
IF SPRITEWIDTH(enemy_sprite) > 100 THEN
    PRINT "Large enemy"
ENDIF
```

**See also:**
\ref SPRITEHEIGHT "SPRITEHEIGHT"
