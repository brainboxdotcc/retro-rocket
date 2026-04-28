\page CHAIN CHAIN Keyword

```basic
CHAIN string-expression[,boolean-expression] [MEMORY SMALL|MEDIUM|LARGE|HUGE]
```

Starts a new BASIC program, pausing the current program until the new one finishes.
The string expression must evaluate to the **filename** of a program accessible from the filesystem.

* Any variables marked as \ref GLOBAL "GLOBAL" will be shared with the child program.
* When the chained program terminates, control returns to the line immediately following the `CHAIN` statement in the calling program.
* Variables not marked as global are not visible to the chained program.
* If the optional boolean expression is included and evaluates to true, the program will be launched as a task in the background and the caller will resume immediately.
* The optional `MEMORY` clause selects the memory model used by the new program.

---

##### Memory Models

The `MEMORY` option controls the size of the allocator used by the child program:

* `SMALL`   — up to 32 MiB per allocation
* `MEDIUM`  — up to 64 MiB per allocation *(default)*
* `LARGE`   — up to 128 MiB per allocation
* `HUGE`    — up to 256 MiB per allocation

If not specified, the child program uses the **same memory model as the caller**, or `MEDIUM` if no parent model applies.

If the requested memory model cannot be allocated, the `CHAIN` will fail with an out-of-memory error.

\remark The memory model does not limit the total memory a program may use.It limits the size of any single contiguous allocation (for example, a string or buffer), and programs may use more memory overall but individual allocations cannot exceed the selected model.

---

\remark `CHAIN` resolves relative paths from the **current working directory**.
\remark Use absolute paths (e.g. `/programs/login`) if you don’t want this to depend on `CHDIR`.

---

##### Examples

```basic
PRINT "Before chain"
GLOBAL X = 42
CHAIN "childprog"
PRINT "Back again, X = "; X
```

```basic
CHAIN "worker", TRUE MEMORY SMALL
```

```basic
CHAIN "bigdata" MEMORY LARGE
```

```basic
CHAIN "analysis" MEMORY HUGE
```

---

##### Notes

* Chained programs run in a **new interpreter instance** within a new process. The parent remains suspended until the child exits, unless the child is launched as a background task.
* A chained program may itself issue `CHAIN` statements, nesting as required.
* If the file specified in `string-expression` cannot be found, an error is raised (and can be caught with `ON ERROR`).
* The memory model determines the **maximum size of any single allocation**, not the total memory usage of the program.
* Unlike BBC BASIC, Retro Rocket’s `CHAIN` does **not** discard the caller - the parent program continues afterwards.

---

**See also:**
\ref GLOBAL "GLOBAL", \ref LIBRARY "LIBRARY"

