\page REPLACE REPLACE\$ Function

#### REPLACE\$

```basic
REPLACE$(haystack-string-expression, needle-string-expression, replacement-string-expression)
```

Replaces every **non-overlapping** occurrence of `needle$` in `haystack$` with `replacement$`.
Matching is **case-sensitive**. The scan proceeds left-to-right and never re-matches text it has just written, so it is safe even when `replacement$` contains `needle$`.

**Notes**

* If `needle$` is `""` (empty), the call is a **no-op** and returns a copy of `haystack$`. This avoids degenerate infinite insertions.
* If `replacement$` is `""`, all occurrences of `needle$` are **removed**.
* Behaviour is **non-overlapping**: after a match, the search continues immediately after the replaced span.
* If `haystack$` is `""`, the function returns `""`.

**Errors**

* **Type mismatch** if any argument is not a string expression.
* **Wrong number of arguments** if fewer or more than three arguments are supplied.

**Examples**

```basic
' Basic replacement (case-sensitive)
PRINT REPLACE$("Hello hello", "he", "X")     ' -> Hello Xllo

' Remove all occurrences
PRINT REPLACE$("abracadabra", "a", "")       ' -> brcdbr

' Replacement contains the needle (safe, non-overlapping)
PRINT REPLACE$("xx", "x", "xx")              ' -> xxxx

' Longer expansion
PRINT REPLACE$("banana", "na", "nana")       ' -> bananana

' Empty needle: no change
PRINT REPLACE$("abc", "", "Z")               ' -> abc
```

**See also**

* \ref INSTR - find substring position
* \ref MID "MID$" - substring by range
* \ref LEFT "LEFT$", \ref RIGHT "RIGHT$" - fixed-width substrings
