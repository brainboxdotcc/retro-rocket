\page RESTRICT RESTRICT Keyword

```basic
RESTRICT keyword-or-function[, keyword-or-function...]
```

Restricts one or more BASIC keywords or built-in functions for any child program created from the current interpreter context.

Once restricted, those keywords or built-in functions cannot be used by child programs inheriting the restriction set. If a child program attempts to use one, execution stops with an error.

### Notes

`RESTRICT` affects child interpreter contexts, not the current program.

Each item must be the name of a valid BASIC keyword or built-in function.

Multiple items may be restricted in a single statement by separating them with commas.

Restrictions are inherited when passed to a child context. A child may also apply further restrictions to its own descendants.

To remove a restriction from the child restriction set, use `DERESTRICT`.

### Errors

An error is raised if:

* no keyword or built-in function is provided
* a named item is not a valid keyword or built-in function
* memory allocation fails while storing the restriction

A child program that attempts to use a restricted keyword or built-in function will also raise an error.

### Examples

```basic
RESTRICT IF
```

```basic
RESTRICT OPENIN, GETNAMECOUNT, VDU
```

```basic
RESTRICT PRINT

CHAIN "/programs/untrusted"
```

### See also

\ref DERESTRICT "DERESTRICT", \ref CHAIN "CHAIN", \ref PROC "PROC", \ref FN "FN"
