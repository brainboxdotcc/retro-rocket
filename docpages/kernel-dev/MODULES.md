\page modules Writing Kernel Modules for Retro Rocket

[TOC]

Retro Rocket supports dynamically loadable kernel modules (`.ko` files).
Modules let you extend the kernel at run-time without rebuilding or rebooting, using a simple ELF64 relocation and symbol resolution system.

---

## Module ABI

Each kernel build exports a single **module ABI version**:

```c
#define KMOD_ABI 100
```

Modules must define ABI-versioned entry points:

```c
bool EXPORTED mod_init_v100(void);
void EXPORTED mod_exit_v100(void);
```

The loader enforces this naming convention.
If the expected `mod_init_v<ABI>` symbol cannot be found, the module is rejected.

*No forward/backward compatibility is provided.*
If the ABI number changes, modules must be rebuilt.

---

## Minimal Module Example

```c
#include <kernel.h>   /* always include this header */

bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
    dprintf("example.ko: mod_init called!\n");
    return true;
}

void EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
    dprintf("example.ko: mod_exit called!\n");
}
```

---

## Using Kernel Symbols

All kernel symbols intended for modules are exported via `kernel.h`.
Modules should **only ever** include:

```c
#include <kernel.h>
```

After that, you can call any exported kernel function directly:

```c
bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
    char *buf = kmalloc(128);
    dprintf("allocated 128 bytes at %p\n", buf);
    return true;
}
```

There is **no need** for `extern` declarations or additional headers.
The loader resolves undefined globals against the kernel’s symbol index (`kernel.sym`) at load time.

---

## Memory Model

The Retro Rocket kernel itself is linked into the **higher half** of the address space.
Modules, however, are loaded into the **kernel heap**, which resides in the **lower half**.

* Key implications:

  * Modules are relocated into an arbitrary heap allocation block.
  * They must **not** assume a fixed or “higher-half” load address.
  * Absolute addresses in module code are always patched by relocations.
  * Pointers into the module’s own image are valid only while the module remains loaded.

---

## Build System

Modules are compiled as **relocatable ELF objects**:

* `ld -r` (ET\_REL output)
* Flags: `-ffreestanding -fno-pic -mcmodel=large -mno-red-zone`

CMake support exists in the Retro Rocket tree:
any sources under `modules/<name>/*.c` are built into `iso/system/modules/<name>.ko`.

---

## Loader Pipeline

The in-kernel loader performs:

1. Parse ELF header and section table (`ET_REL`, `EM_X86_64` only).
2. Allocate one contiguous block for all `SHF_ALLOC` sections with proper alignment.
3. Copy `PROGBITS` data, zero `NOBITS`.
4. Load `.symtab`/`.strtab` for the module.
5. Apply relocations (`R_X86_64_64`, `PC32`, `PLT32`, `32`, `32S`).
6. Resolve externals against `kernel.sym`.
7. Locate `mod_init_v<ABI>`/`mod_exit_v<ABI>`.
8. Call `mod_init_v<ABI>`.

On unload, the loader calls `mod_exit_v<ABI>`, frees the section block, and removes the module from the registry.

---

## Module Lifecycle

* **Load**:
  `load_module("example");`
  → Calls `mod_init_v100()`

* **Unload**:
  `unload_module("example");`
  → Calls `mod_exit_v100()` and frees resources

All modules are tracked in a kernel hashmap keyed by `name`.

---

## Safety Notes

* A failed symbol resolution or relocation aborts load cleanly.
* After unload, the loader nulls entry points and section pointers.
  Any stale use will dereference `NULL` rather than executing freed code.
* Modules must not assume persistence of pointers returned by `module_section_base()` after unload.

---

## Writing Good Modules

* Always check the current `KMOD_ABI` in `module.h` before building.
* Keep module entry points minimal; heavy initialisation should be factored out.
* Use kernel APIs via `kernel.h` rather than re-implementing libc.
* Clean up everything you allocate in `mod_exit_v<ABI>()`.

