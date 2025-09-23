\page SCROLLREGION SCROLLREGION Keyword

```basic
SCROLLREGION integer-expression, integer-expression
```

Marks a horizontal band of the screen, measured in **text rows**, as scrollable with terminal text regardless of if it contains graphics, or text.
Both `top` and `bottom` values are inclusive.

Any graphics drawn in this region will be shifted up together with the text when the screen scrolls.
The region remains active until it has completely scrolled off the display, even if the program that created it has finished.

---

##### Example: Keep a logo and system info scrolling with text

```basic
SPRITELOAD owl,"/images/owl.png"

PRINT " "
PRINT " "
PRINT " "

CURSOR 0, CURRENTY - 3
STARTY = CURRENTY

PLOT owl, 32, STARTY*8
PRINT "Retro Rocket 1.0"
PRINT "CPU: "; CPUGETBRAND$(1)
PRINT "Memory: "; MEMUSED/1024/1024; " MiB"

ENDY = CURRENTY
SCROLLREGION STARTY, ENDY
```

This loads and plots an image, prints a few lines of system information, and then marks the whole block as a scroll region so it moves in sync with the text output.

---

##### Notes

* Coordinates are in **text rows**, not pixels. `(0,0)` is the top-left cell.
* Regions always cover the **full screen width**.
* Multiple regions may be active at the same time.
* Regions are automatically removed once they are entirely outside the visible screen.
* Because of the way the terminal is optimised, **printing blank lines inside a scroll region will appear as vertical bars across images**. When mixing text and graphics in a scroll region, only draw the text you actually want visible.

**See also:**
\ref SPRITELOAD "SPRITELOAD", \ref PLOT "PLOT", \ref PRINT "PRINT"
