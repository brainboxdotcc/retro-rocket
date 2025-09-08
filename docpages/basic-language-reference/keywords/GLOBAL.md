\page GLOBAL GLOBAL Keyword
```basic
GLOBAL variable-name = expression
```

Marks a variable assignment as **global-for-child-processes**. The variable’s **current value is copied** (not referenced) into any **new programs** this program creates (for example via \ref CHAIN "CHAIN"). This mechanism is also how `rocketsh` passes command-line style parameters into a program.


@note `GLOBAL` must appear **with an assignment**.
@note `GLOBAL` **without** an assignment is invalid.

---

### Behaviour

- The assignment executes normally in the current program, **and** the variable is flagged so that its **value is copied** into subsequently created child programs.
- The copy happens **at child creation time**. Changes made later in either parent or child do **not** affect the other.
- To keep a variable marked as global, **every** subsequent assignment to that variable must also be written with `GLOBAL`.
  - If you assign to the variable **without** `GLOBAL`, it will **no longer** be treated as global for future child launches.

---

### Examples

**Pass a string and number to a child**

Parent program:
```basic
GLOBAL USERNAME$ = "guest"
GLOBAL TIMEOUT   = 30

CHAIN "childprog"
PRINT "Back in parent."
```

Child program (`childprog`):
```basic
PRINT "User: "; USERNAME$
PRINT "Timeout: "; TIMEOUT
```

**Losing the global mark by plain assignment**

```basic
GLOBAL MODE$ = "SAFE"
MODE$ = "FAST"
CHAIN "worker"
```

To keep `MODE$` global for future children:
```basic
GLOBAL MODE$ = "FAST"
```

---

### Notes

- `GLOBAL` controls **inter-program passing only**. It does **not** create a program-wide scope within a single program.
- The variable is copied **by value** into the child at launch.
- Use `GLOBAL` only on lines that **assign** to the variable.

**See also:**  
\ref CHAIN "CHAIN"
