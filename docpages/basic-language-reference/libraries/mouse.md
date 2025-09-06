\page mouse Mouse Library

```basic
LIBRARY LIB$ + "/mouse"
```

The Mouse library provides a simple interface to the PS/2 mouse task.

You must start the ps2mouse task before using the library:

```
ROCKETSH> task drivers/ps2mouse
```

The following publicly documented procedures and functions are available via this library.

### PROCmouse_done
uninitialise the library and release resources. Call this when your program is finished with the mouse.

### PROCfetch_mouse
request the current mouse state from the ps2mouse task and update the library’s cached values. Call this regularly in your main loop.

### FNmouse_x()
get the latest absolute X coordinate (integer, pixels). Already clamped to the current screen width.

### FNmouse_y()
get the latest absolute Y coordinate (integer, pixels). Already clamped to the current screen height.

### FNmouse_lmb()
return TRUE if the left mouse button is pressed, else FALSE.

### FNmouse_rmb()
return TRUE if the right mouse button is pressed, else FALSE.

### FNmouse_mmb()
return TRUE if the middle mouse button is pressed, else FALSE.

---

#### Minimal example

```basic
    CLS
    LIBRARY LIB$ + "/mouse"

    SPRITELOAD cursor,"/images/cursor.png"

    X = 0
    Y = 0
    PROCmouse
    REPEAT
        PROCfetch_mouse
        NEW_X = FNmouse_x()
        NEW_Y = FNmouse_y()
        IF NEW_X <> X OR NEW_Y <> Y THEN
            GCOL 0
            RECTANGLE X, Y, X + 32, Y + 45
            X = NEW_X
            Y = NEW_Y
            PLOT cursor, X, Y
        ENDIF
    UNTIL INKEY$ <> ""
    PROCmouse_done
    END
```