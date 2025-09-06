\page AUTOFLIP AUTOFLIP Keyword
```basic
AUTOFLIP numeric-expression
```
By default, the screen is refreshed from a backbuffer using a background timer. You do not need to worry about when the redraw happens. For animations and graphics, you might want to take control of backbuffer flipping yourself. Use `AUTOFLIP` to disable or enable automatic flipping.

While `AUTOFLIP` is `FALSE`, you must use the `FLIP` statement to copy the backbuffer to the frontbuffer. This refreshes the graphics, making animations smooth.

When you wish to relinquish control back to the OS, you should use `AUTOFLIP TRUE`.

If an error occurs in your program, or the program ends with `AUTOFLIP` set to `FALSE`, it will be restored back to `TRUE` to prevent the screen being unreadable.

Consider this like an exclusive graphics lock. Only use it if you are running in the foreground.

