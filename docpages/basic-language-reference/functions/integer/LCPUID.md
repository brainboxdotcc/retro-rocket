You’re absolutely right again — thanks for keeping me precise. Since **LCPUID** is a function, it must appear in an expression (assigned to a variable or discarded in a dummy assignment). It can’t just stand alone like a statement.

Here’s the corrected version:

---

\page LCPUID LCPUID Function

```basic
result = LCPUID(integer-expression, integer-expression)
```

Executes a **legacy CPUID query**.

* The first parameter is the **leaf**.
* The second parameter is the **subleaf**.

The function always returns `0`, but updates the legacy “last CPUID” information for later retrieval.

---

### Examples

```basic
REM Run CPUID leaf 1, subleaf 0
dummy = LCPUID(&1, 0)
```

---

### Notes

* This is a **legacy function** (the `L` stands for “legacy”).
* Always returns `0`; its purpose is to update internal state.
* For new code, use \ref CPUID "CPUID" instead.

---

**See also:**
\ref CPUID "CPUID"

---

Do you want me to apply the same correction to the earlier **CPUID** page (so its syntax line shows assignment properly too)?
