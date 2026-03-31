\page ROTATE ROTATE Keyword

```basic
ROTATE sprite-handle
```

Rotates a sprite **90 degrees clockwise in-place**.

The sprite’s **pixel data and mask** are rotated, and its **width and height are updated accordingly**.

---

##### Examples

**Basic usage**

```basic
SPRITELOAD S,"/images/ship.gif"
PRINT SPRITEWIDTH(S),SPRITEHEIGHT(S)

ROTATE S

PRINT SPRITEWIDTH(S),SPRITEHEIGHT(S)
```

---

**Multiple rotations**

```basic
SPRITELOAD S,"/images/enemy.gif"

ROTATE S    ' 90 degrees
ROTATE S    ' 180 degrees
ROTATE S    ' 270 degrees
ROTATE S    ' back to original orientation
```

---

**Preparing a level image**

```basic
SPRITELOAD LEVEL,"/levels/level1.gif"

ROTATE LEVEL
```

After rotation, rows of the sprite can be treated as **time slices** using \ref SPRITEROW "SPRITEROW".

---

##### Notes

* Rotation is **clockwise only**
* Each call rotates the sprite by **90 degrees**
* The sprite’s dimensions are updated:
  * new width = previous height
  * new height = previous width
* Rotation modifies the sprite **permanently** (until reloaded)
* Both **pixel data and transparency mask** are rotated
* **Animated sprites are not supported** and will raise a runtime error
* Works on any loaded sprite with valid pixel data

---

##### Behaviour summary

| Before     | After ROTATE |
| ---------- | ------------ |
| width = W  | width = H    |
| height = H | height = W   |

---

**See also:**
\ref SPRITELOAD "SPRITELOAD"
\ref SPRITEROW "SPRITEROW"
\ref SPRITEWIDTH "SPRITEWIDTH"
\ref SPRITEHEIGHT "SPRITEHEIGHT"
