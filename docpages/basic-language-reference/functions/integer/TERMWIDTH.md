\page TERMWIDTH TERMWIDTH Function

```basic
TERMWIDTH
```

Returns the **width** of the current terminal, measured in **character columns**.

---

### Examples

```basic
PRINT "Terminal width = "; TERMWIDTH
```

```basic
REM Draw a border along the top of the screen
CURSOR 1, 1
PRINT STRING$(TERMWIDTH, "-")
```

```basic
REM Centre text horizontally
midX = TERMWIDTH / 2 - (LEN("Hello, world!") / 2)
CURSOR midX, TERMHEIGHT / 2
PRINT "Hello, world!"
```

---

### Notes

* Value is returned as an integer ≥ 1.
* Useful for horizontal positioning with \ref CURSOR "CURSOR".
* Works together with \ref TERMHEIGHT "TERMHEIGHT" to adapt output to the current terminal size.

---

**See also:**
\ref TERMHEIGHT "TERMHEIGHT" · \ref CURSOR "CURSOR" · \ref CURRENTX "CURRENTX" · \ref CURRENTY "CURRENTY"
