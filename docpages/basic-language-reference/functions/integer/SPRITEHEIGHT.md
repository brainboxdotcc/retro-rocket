\page SPRITEHEIGHT SPRITEHEIGHT Function

```basic
SPRITEHEIGHT(sprite-handle)
```

Returns the **height** of a sprite in pixels.

---

### Examples

```basic
h = SPRITEHEIGHT(player_sprite)
PRINT "Height: "; h
```

```basic
IF SPRITEHEIGHT(enemy_sprite) > 100 THEN
    PRINT "Tall enemy"
ENDIF
```

**See also:**
\ref SPRITEWIDTH "SPRITEWIDTH"
