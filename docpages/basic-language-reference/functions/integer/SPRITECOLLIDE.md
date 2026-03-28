\page SPRITECOLLIDE SPRITECOLLIDE Function

SPRITECOLLIDE(sprite_a, ax, ay, sprite_b, bx, by)

Checks whether two sprites collide using pixel-perfect collision detection.

Returns TRUE if any opaque pixels overlap, or FALSE if they do not.


---

Examples

```basic
REM Simple collision check
IF SPRITECOLLIDE(player, px, py, enemy, ex, ey) THEN
    PRINT "Hit!"
ENDIF
```

```basic
REM Bullet collision
FOR i = 0 TO bullet_count - 1
    IF SPRITECOLLIDE(bullet(i), bx(i), by(i), enemy, ex, ey) THEN
        PRINT "Enemy hit"
    ENDIF
NEXT
```

---

Notes

Uses sprite transparency for collision.

Sprites must be loaded with \ref SPRITELOAD "SPRITELOAD".



---

See also: \ref SPRITELOAD "SPRITELOAD" · \ref PLOT "PLOT" · \ref ANIMATE "ANIMATE"
