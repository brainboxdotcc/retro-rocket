\page PLOTQUAD PLOTQUAD Keyword
```basic
PLOTQUAD integer-variable, x0, y0, x1, y1, x2, y2, x3, y3
```

Maps a previously loaded **sprite** onto the **quadrilateral** defined by four X,Y coordinate pairs.  
The first parameter must be an **integer variable** containing the sprite handle returned by [`SPRITELOAD`](https://github.com/brainboxdotcc/retro-rocket/wiki/SPRITELOAD).  
The sprite’s full image is used, positioned by its four destination corners.

![example](https://github.com/user-attachments/assets/d6ae6888-46db-42ee-be95-d96c888dc1b5)

---

### Key behaviour

- **Projective mapping**: uses a perspective-correct transform (homography), not a simple affine stretch.
- **Convex quads** only: self-intersecting or zero-area quads have undefined results.
- **Point order**: clockwise or anticlockwise is accepted as long as the quad is convex.
- **Clipping**: points may be off-screen; only visible pixels are drawn.
- **Sprite handle**: the first argument must be an **integer variable** holding a valid handle from `SPRITELOAD`.


> Transparency semantics match `PLOT`. There is no alpha blending.

---

### Parameters

- `integer-variable` - variable containing a sprite handle from `SPRITELOAD`.
- `x0, y0, x1, y1, x2, y2, x3, y3` - integer expressions for the quad’s four destination corners in screen pixels.

---

### Performance notes

- Cost is roughly proportional to the quad’s **screen area** and is naturally slower than `PLOT`.
- There is **no Z-buffer**. When drawing overlapping quads in one frame, the **last drawn** appears on top.

---

### Sampling

- Uses **nearest-neighbour** sampling. Large upscales will look blocky; use higher-resolution sprites if you need crisper results.

---

### Examples

```basic
REM Upright billboard
PLOTQUAD s, 100,60, 220,60, 220,220, 100,220

REM Foreshortened floor tile
PLOTQUAD s, 60,180, 260,180, 220,120, 100,110

REM Diamond / 45 degree rotation
PLOTQUAD s, 160,60, 260,140, 160,220, 60,140
```

---

**See also:**  
[`SPRITELOAD`](https://github.com/brainboxdotcc/retro-rocket/wiki/SPRITELOAD) ·
[`PLOT`](https://github.com/brainboxdotcc/retro-rocket/wiki/PLOT) ·
[`SPRITEFREE`](https://github.com/brainboxdotcc/retro-rocket/wiki/SPRITEFREE)
