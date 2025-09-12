\page CPUID CPUID Function

```basic
CPUID(leaf, subleaf, register-index)
```

Returns a **32-bit register value** from the CPU’s `CPUID` instruction.

* `leaf` (EAX input) selects the **query** (e.g., feature flags, topology).
* `subleaf` (ECX input) refines the query for some leaves (set to `0` if unused).
* `register-index` chooses which returned register to read:

| Value | Register |
| ----- | -------- |
| 0     | EAX      |
| 1     | EBX      |
| 2     | ECX      |
| 3     | EDX      |

If `register-index` is outside `0–3`, an **error** is raised.

For a survey of leaves/subleaves and their meanings, see the **cpuid** command’s reference.

---

### Examples

```basic
REM Print the highest supported standard leaf
max_leaf = CPUID(&00000000, 0, 0)  REM EAX
PRINT "Max standard leaf = "; max_leaf
```

```basic
REM Detect SSE2 (leaf 1, EDX bit 26)
features_edx = CPUID(&00000001, 0, 3)
IF BITAND(features_edx, &04000000) <> 0 THEN
    PRINT "SSE2 supported"
ELSE
    PRINT "SSE2 NOT supported"
ENDIF
```

```basic
REM Detect AVX (leaf 1, ECX bit 28) and OSXSAVE (leaf 1, ECX bit 27)
features_ecx = CPUID(&00000001, 0, 2)
has_osxsave = BITAND(features_ecx, &08000000)    <> 0  REM bit 27
has_avx     = BITAND(features_ecx, &10000000)    <> 0  REM bit 28
IF has_osxsave = TRUE THEN
    IF has_avx = TRUE THEN
        PRINT "AVX supported (CPU advertises AVX and OSXSAVE)"
    ELSE
        PRINT "No AVX (CPU missing AVX bit)"
    ENDIF
ELSE
    PRINT "No AVX usable (OSXSAVE not set)"
ENDIF
```

```basic
REM Detect BMI2 (leaf 7 subleaf 0, EBX bit 8)
ebx_7_0 = CPUID(&00000007, 0, 1)
IF BITAND(ebx_7_0, &00000100) <> 0 THEN
    PRINT "BMI2 supported"
ELSE
    PRINT "BMI2 NOT supported"
ENDIF
```

```basic
REM Print the highest supported extended leaf
max_ext = CPUID(&80000000, 0, 0)  REM EAX
PRINT "Max extended leaf = "; max_ext
```

---

### Notes

* **Return width:** The function returns a Retro Rocket **integer** (64-bit signed),
  but only the **low 32 bits** are meaningful; the result is the raw register value.
* **Signed vs unsigned:** Some feature bits set the high bit; if you print the value
  as a signed decimal it may appear negative. Use **hex masks** (e.g., `&10000000`)
  with `BITAND` in tests, as shown in the examples.
* **Subleaf usage:** Many leaves **ignore** `subleaf`; pass `0` unless the leaf
  explicitly defines subleaf semantics (e.g., leaves `&00000004`, `&0000000B`,
  `&00000007`, `&8000001D`, etc.).
* **Determinism:** `CPUID` serialises certain execution on many CPUs and is safe
  to call at any time from BASIC programs. It has **no side effects** in Retro Rocket.
* **Platform baseline:** Retro Rocket targets x86-64 where `CPUID` is available.
  No compatibility shim is provided for pre-`CPUID` 80386/early 80486 systems.
* **Reading multi-word strings:** Some CPUID leaves (e.g., brand string at
  `&80000002..&80000004`) return **packed ASCII** in four registers per leaf.
  `CPUID` gives you the raw 32-bit words; if you wish to render them as text, extract
  bytes with bit masks and compose characters with `CHR$`. (Feature bit probing, as
  above, is usually what you want inside BASIC.)

---

**See also:**
\ref BITAND "BITAND" · \ref BITOR "BITOR" · \ref BITNAND "BITNAND" · \ref cpuid "cpuid (command)"
