\page DERESTRICT DERESTRICT Keyword

```basic
DERESTRICT keyword-or-function[, keyword-or-function...]
```

Removes one or more previously applied restrictions from the child restriction set.

After a restriction is removed, future child programs created from the current interpreter context are again permitted to use those keywords or built-in functions.

### Notes

`DERESTRICT` modifies the set of restrictions that will be passed to child interpreter contexts. It does not affect restrictions already active in existing child programs.

Each item must be the name of a valid BASIC keyword or built-in function.

Multiple items may be removed in a single statement by separating them with commas.

If a restriction is removed, it simply ceases to be inherited by future child contexts.

### Errors

An error is raised if:

* no keyword or built-in function is provided
* a named item is not a valid keyword or built-in function
* no restrictions exist to remove from
* a named item was not previously restricted

### Examples

```basic
DERESTRICT IF
```

```basic
DERESTRICT OPENIN, GETNAMECOUNT
```

```basic
RESTRICT PRINT, INPUT
DERESTRICT INPUT

CHAIN "/programs/untrusted"
```

### See also

\ref RESTRICT "RESTRICT", \ref CHAIN "CHAIN", \ref PROC "PROC", \ref FN "FN"
