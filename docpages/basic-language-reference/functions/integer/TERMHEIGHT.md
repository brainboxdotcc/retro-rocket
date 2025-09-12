\page TERMHEIGHT TERMHEIGHT Function

```basic
TERMHEIGHT
```

Returns the **height** of the current terminal, measured in **character rows**.

---

### Examples

```basic
PRINT "Terminal height = "; TERMHEIGHT
```

```basic
REM Draw a border along the bottom of the screen
CURSOR 1, TERMHEIGHT
PRINT STRING$(TERMWIDTH, "-")
```

```basic
REM Centre text vertically
midY = TERMHEIGHT / 2
CURSOR 1, midY
PRINT "Hello, world!"
```

---

### Notes

* Value is returned as an integer ≥ 1.
* Useful for positioning with \ref CURSOR "CURSOR".
* Works together with \ref TERMWIDTH "TERMWIDTH" to adapt output to the current terminal size.

---

**See also:**
\ref TERMWIDTH "TERMWIDTH" · \ref CURSOR "CURSOR" · \ref CURRENTX "CURRENTX" · \ref CURRENTY "CURRENTY"
