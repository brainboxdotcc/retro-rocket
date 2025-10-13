\page MATCH MATCH Keyword

```basic
MATCH result, pattern$, haystack$
MATCH result, pattern$, haystack$, var1$, var2$, ...
```

Evaluates a **POSIX ERE** (extended regular expression) against a string.

* In the first form, stores **1** for a match or **0** for no match into `result`.
* In the second form, also assigns text captured by **parenthesised sub-expressions** to additional string variables (`var1$`, `var2$`, …).

\remark Matching is **ASCII-only** (no locale or Unicode).
\remark All regular expressions follow **POSIX ERE** syntax.

---

### Forms

#### Boolean match

```basic
MATCH result, pattern$, haystack$
```

* `result` must be an **integer** variable.
* `pattern$` and `haystack$` are **strings**.
* Returns 1 for a match, 0 for no match.

#### Match with captures

```basic
MATCH result, pattern$, haystack$, cap1$, cap2$, ...
```

* Each parenthesised group in `pattern$` (e.g. `(abc)`) is captured and copied into successive string variables.
* Missing or non-participating groups yield `""`.
* If the pattern contains fewer capture groups than variables, the extras receive empty strings.

---

### Examples

**Simple match**

```basic
MATCH R, "HELLO", "HELLO WORLD"
IF R THEN PRINT "Found"
```

**Anchors**

```basic
MATCH R, "^START", "START HERE"
PRINT R          ' 1

MATCH R, "END$", "THE END"
PRINT R          ' 1
```

**Alternation and character classes**

```basic
MATCH R, "CAT|DOG", "HOTDOG"
PRINT R          ' 1

MATCH R, "[A-Z][a-z]+", "Title"
PRINT R          ' 1
```

**Capturing sub-expressions**

```basic
MATCH R, "([A-Za-z]+),([A-Za-z]+)", "Hello,World", FIRST$, SECOND$
PRINT R, FIRST$, SECOND$   ' 1  Hello  World
```

**No match clears captures**

```basic
MATCH R, "(\d+)", "No digits here", NUM$
PRINT R, NUM$              ' 0  ""
```

**Invalid pattern handling**

```basic
ON ERROR PROCbad
MATCH R, "(?i)HELLO", "hello"   ' invalid: (?i) not supported
PRINT "this line is not reached"
END

DEF PROCbad
PRINT "Regex error!"
END
```

---

### Supported syntax (POSIX ERE subset)

| Feature           | Example                    | Description                  |              |
| ----------------- | -------------------------- | ---------------------------- | ------------ |
| Literals          | `ABC`                      | exact characters             |              |
| Any char          | `.`                        | matches any single character |              |
| Quantifiers       | `* + ?`                    | greedy repetition            |              |
| Character classes | `[abc]`, `[A-Z]`, `[^0-9]` | set, range, negation         |              |
| Alternation       | `A                         | B`                           | match A or B |
| Anchors           | `^`, `$`                   | start / end of string        |              |
| Capturing groups  | `(ABC)`                    | capture substring            |              |

---

### Not supported

* Backreferences `\1`, `\2`, …
* Inline flags `(?i)` etc.
* PCRE-style escapes (`\d`, `\w`, lookaround, …)
* Multiline mode (`^` and `$` match string boundaries only)

---

### Notes

* Matching is **case-sensitive**. To simulate case-insensitive matching, normalise both strings or use explicit character classes.
* With captures, **co-operative execution is disabled** - the operation completes immediately.
* Without captures, matching runs **co-operatively** across idle ticks for long inputs.
* If the pattern is invalid, the engine reports a descriptive message.
  Without an error handler, the program terminates;
  with `ON ERROR PROCname`, control transfers to the handler.
* Capture results are always independent copies; modifying the original string has no effect on captured values.
