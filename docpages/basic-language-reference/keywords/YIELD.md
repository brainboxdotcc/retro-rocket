\page YIELD YIELD Keyword
```basic
YIELD
```


@note **Deprecated / discouraged.**
@note `YIELD` is not required in Retro Rocket BASIC and may be removed. It does not solve any real problem and simply wastes time.

`YIELD` was intended as a hint to let other work run, but Retro Rocket’s scheduler already **pre-empts BASIC automatically at the end of every line**.  
A BASIC program **cannot monopolise the CPU**, so inserting `YIELD` calls has **no scheduling benefit**.

---

### Behaviour

- Executes as a **no-op** for control flow and timing; it does not pause or advance timers.
- Does **not** improve responsiveness; the interpreter yields naturally after each completed line.
- Adds a small amount of overhead if sprinkled inside loops.

---

### Example (for illustration only)

```basic
PRINT "A"
YIELD
PRINT "B"
```

The output and timing are effectively the same as omitting `YIELD`.

---

### Alternatives

- If you want an **actual pause**, use \ref SLEEP "SLEEP" (seconds resolution).