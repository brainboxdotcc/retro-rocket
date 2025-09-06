\page SPRITEFREE SPRITEFREE Keyword
```basic
SPRITEFREE integer-expression
```

Unloads a **sprite** whose handle is given by `integer-expression`.  
After `SPRITEFREE`, the handle is no longer valid for drawing.


> Each program may have **up to 1024 sprites** loaded at any one time.  
> Sprite handles are **per-program**, and all remaining sprites are **automatically freed** when the program ends.


> Passing an invalid handle (or one that has already been freed) raises a runtime error  
> that you can catch with [`ON ERROR`](https://github.com/brainboxdotcc/retro-rocket/wiki/ONERROR).

---

### Examples

**Load, draw, then free**
```basic
SPRITELOAD S, "/images/logo.png"
PLOT S, 100, 80
SPRITEFREE S
```

**Free several sprites**
```basic
SPRITELOAD A, "/images/a.png"
SPRITELOAD B, "/images/b.png"
SPRITELOAD C, "/images/c.png"

PLOT A, 0, 0
PLOT B, 64, 0
PLOT C, 128, 0

SPRITEFREE A
SPRITEFREE B
SPRITEFREE C
```

**Free from an array of handles**
```basic
DIM H,3
SPRITELOAD H(0), "/images/one.png"
SPRITELOAD H(1), "/images/two.png"
SPRITELOAD H(2), "/images/three.png"

FOR I = 0 TO 2
    SPRITEFREE H(I)
NEXT
```

**Handle errors**
```basic
ON ERROR PROCerr
SPRITEFREE 9999
PRINT "This line is reached only if no error occurs"
END

DEF PROCerr
    PRINT "SPRITEFREE failed: "; ERR$
    ON ERROR PROCerr
ENDPROC
```

---

### Notes
- `integer-expression` can be a variable, array element, or any expression that evaluates to a valid sprite handle.
- Once freed, do not use the handle with `PLOT` or `PLOTQUAD` unless you load a new sprite and store its new handle.
- Freeing resources explicitly during long-running programs helps control memory use.

**See also:**  
[`SPRITELOAD`](https://github.com/brainboxdotcc/retro-rocket/wiki/SPRITELOAD) ·
[`PLOT`](https://github.com/brainboxdotcc/retro-rocket/wiki/PLOT) ·
[`PLOTQUAD`](https://github.com/brainboxdotcc/retro-rocket/wiki/PLOTQUAD)
