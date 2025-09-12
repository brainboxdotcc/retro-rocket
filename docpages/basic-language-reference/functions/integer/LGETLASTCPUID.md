\page LGETLASTCPUID LGETLASTCPUID Function

```basic
LGETLASTCPUID(integer-expression)
```

Returns the result of the most recent **legacy CPUID** query, previously initiated by \ref LCPUID "LCPUID".

The single parameter is an **index** selecting which register value to return, as documented on the \ref CPUID "CPUID" function page:

| Value | Register |
| ----- | -------- |
| 0     | EAX      |
| 1     | EBX      |
| 2     | ECX      |
| 3     | EDX      |

---

### Examples

```basic
REM Run CPUID leaf 1, subleaf 0
dummy = LCPUID(&1, 0)

REM Retrieve the EAX result
eax = LGETLASTCPUID(0)
PRINT "EAX = "; eax
```

```basic
REM Print all four registers from a legacy query
dummy = LCPUID(&1, 0)
FOR r = 0 TO 3
    PRINT "Reg("; r; ") = "; LGETLASTCPUID(r)
NEXT
```

---

### Notes

* Returns a 32-bit register value as an integer.
* Only valid after a preceding call to \ref LCPUID "LCPUID"; otherwise the results are undefined.
* Passing an index outside `0–3` raises an error.
* Provided for **compatibility** with older programs that used legacy CPUID access. New code should use \ref CPUID "CPUID" directly.

---

**See also:**
\ref LCPUID "LCPUID" · \ref CPUID "CPUID"
