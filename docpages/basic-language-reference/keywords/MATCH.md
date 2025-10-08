\page MATCH MATCH Keyword

```basic
MATCH result, pattern$, haystack$
```

Evaluates a **POSIX ERE** (extended regular expression) against a string and stores **1** for a match or **0** for no match into `result`.

* `result` must be an **integer** variable.
* `pattern$` and `haystack$` are **strings**.
* Matching is **ASCII-only** (no locale/Unicode).
* No capture groups or sub-matches are returned; this is a **yes/no** test.

`MATCH` runs **cooperatively**: very large or pathological patterns are executed in slices.

\remark If the pattern is invalid, an error is raised with a descriptive message from the regex engine. Without an error handler, the program terminates. With an `ON ERROR` handler, control passes there.

---

### Supported syntax (POSIX ERE subset)

* Literals: `ABC`
* Any char: `.`
* Quantifiers: `* + ?` (greedy)
* Character classes: `[abc]`, ranges `[a-z]`, negation `[^0-9]`
* Alternation: `A|B`
* Anchors: `^` (start of string), `$` (end of string)

### Not supported

* Backreferences `\1`, `\2`, …
* Inline flags like `(?i)` (use explicit classes instead, or upper/lower where appropriate)
* PCRE extensions (`\d`, `\w`, lookaround, etc.)
* Multiline mode: `^` and `$` match **string** boundaries only.

---

### Examples

**Simple literal**

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

MATCH R, "^A", "BA"
PRINT R          ' 0
```

**Alternation**

```basic
MATCH R, "CAT|DOG", "HOTDOG"
PRINT R          ' 1

MATCH R, "RED|GREEN", "BLUE"
PRINT R          ' 0
```

**Character classes and ranges**

```basic
MATCH R, "[0-9]+", "foo123bar"
PRINT R          ' 1

MATCH R, "[A-Z][a-z]+", "Title"
PRINT R          ' 1

MATCH R, "[^x]*z$", "crab ballz"
PRINT R          ' 1
```

**Wildcard and quantifiers**

```basic
MATCH R, "A.*C", "AXYZC"
PRINT R          ' 1

MATCH R, "A.+C", "AC"
PRINT R          ' 0

MATCH R, "B*", "AAAA"
PRINT R          ' 1   ' empty match is allowed
```

**Handling invalid patterns**

```basic
ON ERROR GOTO BAD
MATCH R, "(?i)HELLO", "hello"   ' invalid: (?i) not supported
PRINT "this line is not reached"

BAD:
PRINT "Regex error!"
RESUME NEXT
```

---

### Notes

* Matching is **case-sensitive** by default. To approximate case-insensitive tests, normalise your data (e.g., convert both strings to upper case before matching) or use character classes (e.g., `[Hh][Ee][Ll][Ll][Oo]`).
* Because `MATCH` is cooperative, very large inputs or patterns may take multiple idle ticks to complete. You do not need to poll—control returns to your program automatically once finished.
* `^` and `$` are **string** anchors, not line anchors; there is no multiline mode.
* The engine is compiled with `REG_NOSUB`; capture offsets are not available to BASIC code.
