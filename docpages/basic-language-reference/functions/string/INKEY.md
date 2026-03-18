\page INKEY INKEY$ Function

```basic
A$= INKEY$(STRING)
```

INKEY$ checks for key state on the keyboard. It has two modes of operation:

- If the string parameter passed is an empty string, it will check if any key is waiting in the keyboard buffer, consuming it from the buffer and returning it, or returning an empty string if no key is available.
- If the string parameter passed in is a non-empty single-character string, it will check if that key is currently held, and will not consume it from the buffer. Because this form of the function
  is not consuming from the buffer you may use this form to concurrently check for multiple depressed keys at the same time. Note that you must still drain the buffer. Use a separate INKEY$("")
  call to drain any pending key presses so the buffer does not fill. **US-ASCII values (A-Z) are considered case-insensitive** in the parameter when using this form of `INKEY$`.
- When `INKEY$` consumes input from the **keyboard buffer** by using an empty parameter, if multiple keys have been pressed between this call and the last call to `INKEY$` (or an `INPUT`
  statement), it will take multiple calls to retrieve them all.

---

### Extended keys

Some non-printing keys return special character codes rather than ASCII. These values can be retrieved by wrapping `INKEY$` with `ASC()`.

| Key      | `ASC(INKEY$(""))` value |
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
    k$ = INKEY$("")
UNTIL k$ > ""
PRINT "You pressed: "; k$
```

```basic
REM Handle arrow keys
k$ = INKEY$("")
IF k$ > "" THEN
    code = ASC(k$)
    IF code = 250 THEN PRINT "Up arrow pressed"
    IF code = 251 THEN PRINT "Down arrow pressed"
ENDIF
```

```basic
REM multiple key example
PRINT "Press both A and Z together to exit"
REPEAT
UNTIL INKEY$("A") <> "" AND INKEY$("Z") <> ""
END
```

---

### Notes

* Printable characters return their usual ASCII values (e.g. `"A"` → 65).
* Extended keys must be checked via `ASC()`.
* If no key is pressed, the result is `""` (empty string).
* If the parameter to `INKEY$` contains more than one character, any characters except the first are ignored.

---

**See also:**
\ref ASC "ASC" · \ref INPUT "INPUT"
