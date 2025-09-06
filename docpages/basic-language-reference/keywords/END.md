\page END END Keyword
```basic
END
```

Terminates execution of the **current program**, returning control to the context that started it.

---

- **What happens after `END`:**
  - Started via [`CHAIN`](https://github.com/brainboxdotcc/retro-rocket/wiki/CHAIN): the parent program resumes at the statement **after** its `CHAIN`.
  - Started from `rocketsh`: control returns to the **shell prompt**.
  - Initial `/programs/init` and no other programs remain: the system **halts** and reports that **no programs remain**.
  - Nested `CHAIN`s: only the **current child** exits; its **immediate parent** resumes.

---

##### Example

```basic
IF FILETYPE$("config.txt") <> "FILE" THEN
    PRINT "Missing configuration."
    END
ENDIF

PRINT "All good - continue running…"
```

---

##### Notes
- `END` takes **no arguments**.
- Execution stops **immediately**; any statements after `END` are not executed.
