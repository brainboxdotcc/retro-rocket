\page CHAIN CHAIN Keyword
```basic
CHAIN string-expression[,boolean-expression]
```

Starts a new BASIC program, pausing the current program until the new one finishes.  
The string expression must evaluate to the **filename** of a program accessible from the filesystem.

- Any variables marked as [`GLOBAL`](https://github.com/brainboxdotcc/retro-rocket/wiki/GLOBAL) will be shared with the child program.  
- When the chained program terminates, control returns to the line immediately following the `CHAIN` statement in the calling program.  
- Variables not marked as global are not visible to the chained program.
- If the optional boolean expression is included and evaluates to true, the program will be launched as a task in the background and the caller will resume immediately.


\remark `CHAIN` resolves relative paths from the **current working directory**.
\remark Use absolute paths (e.g. `/programs/login`) if you don’t want this to depend on `CHDIR`.

---

##### Example

```basic
PRINT "Before chain"
GLOBAL X = 42
CHAIN "childprog"
PRINT "Back again, X = "; X
```

If `childprog` uses the global variable `X`, it will see the value `42`.  
After `childprog` ends, execution resumes.

---

##### Notes
- Chained programs run in a **new interpreter instance** within a new process. The parent remains suspended until the child exits, unless the child is launched as a background task.
- A chained program may itself issue `CHAIN` statements, nesting as required.  
- If the file specified in `string-expression` cannot be found, an error is raised (and can be caught with [`ON ERROR`](https://github.com/brainboxdotcc/retro-rocket/wiki/ONERROR)).  
- Unlike BBC BASIC, Retro Rocket’s `CHAIN` does **not** discard the caller - the parent program continues afterwards.

**See also:** [`GLOBAL`](https://github.com/brainboxdotcc/retro-rocket/wiki/GLOBAL), [`LIBRARY`](https://github.com/brainboxdotcc/retro-rocket/wiki/LIBRARY)
