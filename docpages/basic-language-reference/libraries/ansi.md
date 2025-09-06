\page ansi ANSI Library

```BASIC
LIBRARY LIB$ + "/ansi"
```

The ANSI library contains various terminal related functions, for example enabling or disabling the cursor, clearing lines, and interactive input.

The following publically doucmented procedures and functions are available via this library.

### PROChideCursor
hide text cursor

### PROCshowCursor
show text cursor

### PROCclearLineFromCursor
Clear the text on the current line from the cursor to the right

### PROCEDabort
Emulate pressing `ESC` while waiting on interactive input

### FNEdResult$()
Get the line editor result text. In the event of an aborted line (`ESC` pressed) this returns an empty string and FNEdAborted() returns `TRUE`.

### FNEdAborted()
Return `TRUE` if line edit was aborted (`ESC` pressed)

### PROCEdLine
Accept line input using line-edit and history. On completion of the procedure, you can obtain the completed line line in FNEdResult$(). In the event the user pressed `ESC`, FNEdAborted() will return `TRUE`

### PROCEDup
Emulate pressing cursor `UP` in the interactive editor. This will navigate through history, if enabled.

### PROCEDdown
Emulate pressing cursor `DOWN` in the interactive editor. This will navigate through history, if enabled.

### PROCEdAddHistory(hist$)
Add a line to the interactive edit history, if interactive history was enabled when calling PROCEdInit(). There is no practical limit on the size of the history, as it will be expanded to accomodate any additional lines added in allocations of 50 possible entries.

### PROCEDleft
Emulate pressing cursor `LEFT` in the interactive editor. The cursor will be moved one character left unless it is at the start of the input.

### PROCEDright
Emulate pressing cursor `RIGHT` in the interactive editor. The cursor will be moved one character right unless it is at the end of the input.

### PROCEDend
Emulate pressing `END` in the interactive editor. The cursor will be moved to the end of the input text.

### PROCEDhome
Emulate pressing `HOME` in the interactive editor. The cursor will be moved to the start of the input text.

REM handle delete edit key
### PROCEDdelete
Emulate pressing delete in the interactive editor. The character to the right, if any, will be deleted.

### PROCEDbackSpace
Emulate pressing backspace in the interactive editor. The character to the left, if any, will be deleted and the cursor advanced one character left.

### PROCEDinsertChar(i)
Emulate inserting an ASCII character whos code is represented by `i` into the interactive editor. If there is space in the current line, the character will be inserted at the cursor position.

### PROCEdRun
Check the input for new keys and insert them into the edit history. This should be called at regular intervals and no other facility should call `INPUT` or `INKEY$` during this time.

### PROCEdInit(history)
initialise interactive line editor. You should call this once, before you call any other interactive line editor procedures or functions.
<table>
<tr><th>history parameter value</th><th>Meaning</th></tr>
<tr><td>TRUE</td><td>Enable and allow use of edit history. Use PROCEdAddEditHistory() to add lines to the history.</td></tr>
<tr><td>FALSE</td><td>Disable and disallow use of edit history. PROCEdAddEditHistory() and cursor `UP` and `DOWN` keys will be ignored.</td></tr>
</table>

### PROCEdStart
Start interactive line input. This is used when you wish to handle the input loop yourself. Line editing begins at the current cursor position and will continue until the end of the current line. For example:

```BASIC
	PROCEdStart
	REPEAT
		PROCEdRun
	UNTIL FNEdResult$() = TRUE
```
