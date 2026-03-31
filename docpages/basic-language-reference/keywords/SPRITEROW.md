\page SPRITEROW SPRITEROW Keyword

```basic
SPRITEROW sprite-handle,y,array-name
```

Copies a single **row of pixel data** from a sprite into an **integer array**.

* `y` is the **row index** (0-based)
* `array-name` is created or resized to the sprite’s width
* Each element in the array contains the **pixel value** at that X position

---

##### Examples

**Basic usage**

```basic
SPRITELOAD S,"/images/maze.gif"

SPRITEROW S,10,ROW

FOR I = 0 TO SPRITEWIDTH(S)-1
    PRINT ROW(I)
NEXT
```

---

**Using with ARRAYFIND**

```basic
SPRITELOAD LEVEL,"/levels/level1.gif"
ROTATE LEVEL

SPRITEROW LEVEL,tick,ROW
ARRAYFIND ROW,RGB(255,0,0),ENEMIES,COUNT

FOR I = 0 TO COUNT-1
    PROCspawn_enemy(ENEMIES(I))
NEXT
```

---

**Reading multiple rows**

```basic
SPRITELOAD S,"/images/data.gif"

FOR Y = 0 TO SPRITEHEIGHT(S)-1
    SPRITEROW S,Y,ROW
    PRINT ROW(0)
NEXT
```

---

##### Notes

* `array-name` is always an **integer array**
* If the array does not exist, it is created using `DIM`
* If it already exists, it is resized using `REDIM`
* Array size is always `SPRITEWIDTH(sprite-handle)`
* Pixel values are returned in the system’s **internal colour format**
* Rows are **0-based** (`0 .. SPRITEHEIGHT-1`)
* Accessing a row outside this range raises a runtime error
* Works with any sprite containing valid pixel data
* **Animated sprites are not supported**

---

##### Behaviour summary

For a sprite of width `W`:

```basic
SPRITEROW S,Y,ROW
```

Produces:

```basic
ROW(0)   = pixel at (0,Y)
ROW(1)   = pixel at (1,Y)
...
ROW(W-1) = pixel at (W-1,Y)
```

---

**See also:**
\ref SPRITELOAD "SPRITELOAD"
\ref ROTATE "ROTATE"
\ref ARRAYFIND "ARRAYFIND"
\ref SPRITEWIDTH "SPRITEWIDTH"
\ref SPRITEHEIGHT "SPRITEHEIGHT"
