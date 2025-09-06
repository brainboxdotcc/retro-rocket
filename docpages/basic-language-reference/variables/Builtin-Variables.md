\page type-bi Built-In Variables

The following variables are pre-defined by Retro Rocket BASIC and cannot be reassigned:

* **`TRUE`** – Always contains the numeric value `1`.  
  Useful for logical expressions, e.g., `IF A = TRUE THEN ...`.

* **`FALSE`** – Always contains the numeric value `0`.  
  Often used as the opposite of `TRUE`, e.g., `IF A = FALSE THEN ...`.

* **`PID`** – Contains the process ID (integer) of the current process.

* **`ERR`** – True only while in an error handler or after an error in an `EVAL`.

* **`ERRLINE`** – Contains the line an error last occurred on while in error handler or after an error in an `EVAL`.

* **`ERR$`** – Contains the error message which last occurred while in error handler or after an error in an `EVAL`.

* **`ARG$`** – A string containing the command line arguments passed to the current process.

* **`PROGRAM$`** – A string containing the fully qualified path of the current program.

* **`GRAPHICS_WIDTH`** – An integer containing the width of the graphics screen

* **`GRAPHICS_HEIGHT`** – An integer containing the height of the graphics screen

* **`GRAPHICS_CENTRE_X`** – An integer containing the X coordinate of the centre of the graphics screen.
  Also accessible via the American English spelling `GRAPHICS_CENTER_X`.

* **`GRAPHICS_CENTRE_Y`** – An integer containing the Y coordinate of the centre of the graphics screen.
  Also accessible via the American English spelling `GRAPHICS_CENTER_Y`.

* **`PI#`** – A real constant for π (`3.141592653589793238`).  
  Useful for trigonometric or geometric calculations.

* **`E#`** – A real constant for e (`2.7182818284590451`).  
  Useful for trigonometric or geometric calculations.

### TRUE and FALSE

In Retro Rocket BASIC, `TRUE` is defined as `1` and `FALSE` as `0`. This differs from BBC BASIC, where `TRUE` was `-1`.

**Why BBC BASIC used `-1`:**  
BBC BASIC, like many early BASIC dialects, used 16-bit signed integers with all bits set (`11111111 11111111`) representing `-1`.  
This meant any logical `NOT` operation (bitwise inversion) would turn `-1` into `0` (FALSE), and vice versa, which was convenient when logical values were treated as bit patterns.

**Why Retro Rocket uses `1`:**  
Retro Rocket BASIC treats boolean values more like modern languages (C, Python, etc.), where `TRUE` is `1` and `FALSE` is `0`.  
This is more intuitive, avoids confusion when mixing booleans with arithmetic, and reflects the way most modern hardware tests nonzero values.


