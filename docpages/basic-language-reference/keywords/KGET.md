\page KGET KGET Keyword
```basic
KGET variable
```
Wait for a single keypress from the user, without echoing it to the terminal. The character code will be placed into the specified variable. String (`$`), integer, and real (`#`) variables are accepted:

- For a string variable (e.g. `a$`), the value will be a one-character string.
- For an integer variable (e.g. `k`), the ASCII code of the key will be stored.
- For a real variable (e.g. `k#`), the ASCII code will be converted to a floating-point value.

Unlike `INKEY$`, `KGET` will block execution until a key is pressed, and is ideal for programs that do not need to poll multiple input sources.


\remark Press `CTRL+ESC` at any time to cancel waiting for input.
\remark Without an error handler, the program terminates.
\remark With an `ON ERROR` handler, control passes there instead.


\remark If you're writing a program that must respond to both keyboard and other input (e.g. network), `INKEY$` remains the preferred choice.
