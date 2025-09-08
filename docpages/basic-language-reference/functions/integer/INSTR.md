\page INSTR INSTR Function

#### INSTR

```basic
INSTR(haystack$, needle$)
```

Searches for the substring `needle$` inside the string `haystack$`.
Returns the 1-based index of the first occurrence, or `0` if not found. The search is case-sensitive.

**Notes**

* The first parameter is the string to search in (the *haystack*).
* The second parameter is the substring to search for (the *needle*).
* If multiple matches exist, only the first occurrence is returned.
* An empty `needle$` always returns `1`.

**Errors**

* None.

**Examples**

@code
PRINT INSTR("HELLO WORLD", "WORLD")   ' Prints: 7
PRINT INSTR("HELLO WORLD", "world")   ' Prints: 0 (case-sensitive)
PRINT INSTR("ABCDE", "A")             ' Prints: 1
PRINT INSTR("ABCDE", "")              ' Prints: 1
@endcode

**See also**

\ref LEFT "LEFT$" · \ref RIGHT "RIGHT$" · \ref MID "MID$" · \ref LEN "LEN$"
