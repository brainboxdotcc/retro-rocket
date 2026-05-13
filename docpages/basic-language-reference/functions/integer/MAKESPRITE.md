\page MAKESPRITE MAKESPRITE Function

```basic
handle = MAKESPRITE(buffer, length)
```

Allocate and initialise a new sprite from image data loaded into a memory buffer

---

### Examples

```basic
sm = MEMALLOC(4096)
f = OPENIN("/images/owl.png")
BINREAD f, sm, 4096
spr = MAKESPRITE(sm, 4096)
MEMRELEASE sm
PLOT spr, 600, 400
```

---

### Notes

* An error will be raised if the memory does not contain valid image data, or is unable to be read

---

**See also:**
\ref SPRITELOAD "SPRITELOAD" · \ref PLOT "PLOT" · \ref PLOTQUAD "PLOTQUAD" · \ref MEMALLOC "MEMALLOC"
