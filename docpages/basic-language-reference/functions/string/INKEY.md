\page INKEY INKEY$ Function

```basic
A$= INKEY$
```

Returns the **key that has been pressed** as a string.
If no key has been pressed since the last call, it returns an **empty string**.

`INKEY$` consumes input from the **keyboard buffer**. If multiple keys have been pressed between this call and the last call to `INKEY$` (or an `INPUT` statement), it will take multiple calls to retrieve them all.

---

### Extended keys

Some non-printing keys return special character codes rather than ASCII.
These values can be retrieved by wrapping `INKEY$` with `ASC()`.

| Key      | `ASC(INKEY$)` value |
| -------- | ------------------- |
| PAGEUP   | 245                 |
| PAGEDOWN | 246                 |
| DEL      | 247                 |
| INS      | 248                 |
| END      | 249                 |
| UP       | 250                 |
| DOWN     | 251                 |
| LEFT     | 252                 |
| RIGHT    | 253                 |
| HOME     | 254                 |

---

### Examples

```basic
REM Wait for a keypress
PRINT "Press a key..."
REPEAT
    k$ = INKEY$
UNTIL k$ > ""
PRINT "You pressed: "; k$
```

```basic
REM Handle arrow keys
k$ = INKEY$
IF k$ > "" THEN
    code = ASC(k$)
    IF code = 250 THEN PRINT "Up arrow pressed"
    IF code = 251 THEN PRINT "Down arrow pressed"
ENDIF
```

---

### Notes

* Printable characters return their usual ASCII values (e.g. `"A"` → 65).
* Extended keys must be checked via `ASC()`.
* If no key is pressed, result is `""` (empty string).

---

**See also:**
\ref ASC "ASC" · \ref INPUT "INPUT"
