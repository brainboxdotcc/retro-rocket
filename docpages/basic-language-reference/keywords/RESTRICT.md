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

`MEMORY` is a special restriction keyword. Unlike its normal use with \ref CHAIN "CHAIN" to select a child program memory model, restricting `MEMORY` enables memory access validation for the child context.

When `MEMORY` is restricted:

* memory reads and writes are limited to regions allocated via \ref MEMALLOC "MEMALLOC"
* arbitrary memory access outside granted regions raises an error
* accessing valid pointers allocated by other BASIC programs raises an error
* this restriction applies to (amongst other keywords) \ref PEEK "PEEK", \ref POKE "POKE" and word, double-word and quad-word variants, \ref BINREAD "BINREAD", \ref BINWRITE "BINWRITE", \ref SOCKBINREAD "SOCKBINREAD", \ref SOCKBINWRITE "SOCKBINWRITE", buffer-to-string conversion, and any other function that directly accesses memory buffers

This allows a parent program to run child programs with restricted direct memory access while still permitting controlled use of dynamically allocated buffers.

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

```basic
RESTRICT MEMORY

CHAIN "/programs/sandbox"
```

### See also

\ref DERESTRICT "DERESTRICT", \ref CHAIN "CHAIN", \ref PROC "PROC", \ref FN "FN", \ref MEMALLOC "MEMALLOC", \ref MEMRELEASE "MEMRELEASE"
