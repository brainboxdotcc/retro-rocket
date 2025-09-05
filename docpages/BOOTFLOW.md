# Retro Rocket Boot Flow

This document describes the execution path from power-on to a running shell in **Retro Rocket** - a modern-day BBC Micro–inspired operating system. It walks through each stage of the boot process, from initial assembly handoff to the multitasking interpreter loop that drives userland programs.

---

## Overview

| Stage                        | Description                                      |
|-----------------------------|--------------------------------------------------|
| `boot.asm`                  | Enables FPU/SSE, calls `kmain()`                |
| `kmain()`                   | Calls `init()` to bring up system subsystems    |
| `init()`                    | Sets up memory, devices, filesystems, etc       |
| `init_process()`            | Creates process from `/programs/init`           |
| `/programs/init` (BASIC)    | Mounts filesystems, sets globals, chains shell  |
| `/programs/rocketsh`        | Shell process - user interaction begins         |

---

## 1. Boot Assembly: `boot.asm`

The kernel is entered via the symbol `boot_bsp`, defined in NASM assembly.

```
bits 64
section .text

extern kmain
extern boot_bsp
extern exception_handlers
extern enable_fpu
extern enable_sse

boot_bsp:
    call enable_fpu
    call enable_sse
    mov rax, kmain
    call rax
    jmp $  ; park the CPU
```

- FPU and SSE are enabled for floating point support
- `kmain()` is the first C-level entry point
- Limine has already set up the page tables and higher-half mapping
- No paging, GDT, or IDT setup is done here - all handled later

---

## 2. Kernel Entry Point: `kmain()`

```c
void kmain()
{
    init();

    if (!filesystem_mount("/", "cd0", "iso9660")) {
        preboot_fail("Failed to mount boot drive to VFS!");
    }

    netdev_t* network = get_active_network_device();
    if (network) {
        kprintf("Active network card: %s\n", network->description);
        network_up();
    }

    init_process();
}
```

- Calls `init()` to bring up subsystems
- Mounts the boot filesystem
- Starts networking drivers
- Enters `init_process()` which boots into userland

---

## 3. Core System Initialisation: `init()`

```c
init_func_t init_funcs[] = {
    init_heap,
    validate_limine_page_tables_and_gdt,
    init_console,
    init_cores,
    init_idt,
    init_pci,
    init_realtime_clock,
    init_devicenames,
    init_keyboard,
    init_ide,
    init_ahci,
    init_filesystem,
    init_iso9660,
    init_devfs,
    init_fat32,
    init_rtl8139,
    init_e1000,
    NULL,
};
```

Each function is executed in sequence with logging.

### Initialisation Order

| Order | Function                       | Purpose                                  |
|-------|--------------------------------|------------------------------------------|
| 1     | `init_heap`                    | Memory allocation                        |
| 2     | `validate_limine_page_tables_and_gdt` | Limine checks                    |
| 3     | `init_console`                 | Terminal setup (Flanterm)                |
| 4     | `init_cores`                   | Single-core bring-up, ACPI               |
| 5     | `init_idt`                     | Interrupt Descriptor Table               |
| 6     | `init_pci`                     | PCI bus scan                             |
| 7     | `init_realtime_clock`          | Realtime wall clock                      |
| 8     | `init_devicenames`             | Friendly device names                    |
| 9     | `init_keyboard`                | Keyboard setup                           |
| 10    | `init_ide`                     | IDE disk support                         |
| 11    | `init_ahci`                    | AHCI disk support                        |
| 12    | `init_filesystem`              | VFS infrastructure                       |
| 13    | `init_iso9660`                 | ISO9660 read-only FS                     |
| 14    | `init_devfs`                   | /devices virtual filesystem              |
| 15    | `init_fat32`                   | FAT32 support                            |
| 16    | `init_rtl8139`                 | Realtek RTL8139 network driver           |
| 17    | `init_e1000`                   | Intel e1000 network driver               |

---

## 4. Process Management and Shell Launch

After filesystems are ready, `init_process()` loads the first userland BASIC program:

```c
void init_process()
{
    process_by_pid = hashmap_new(...);
    process_t* init = proc_load("/programs/init", 0, "/");
    if (!init) {
        preboot_fail("/programs/init missing or invalid!\n");
    }
    proc_loop();
}
```

### `proc_loop()`

```c
void proc_loop()
{
    while (true) {
        proc_timer();
        proc_run_next();
        run_idle_tasks();
    }
}
```

- Every iteration advances the current process
- Each process runs one line of code
- Terminated processes are cleaned up
- System halts if no processes remain

---

## 5. Userland Bootstrap: `/programs/init`

This BASIC program acts like a boot script (like `AUTOEXEC.BAT` or `!Boot`):

```basic
GLOBAL LIB$ = "/programs/libraries"

PRINT "Mounting ";
COLOR 14
PRINT "filesystems";
COLOR 7
PRINT "..."

MOUNT "/devices", "", "devfs"
MOUNT "/harddisk", "hd0", "fat32"

REPEAT
    PRINT "Launching ";
    COLOR 14
    PRINT "shell";
    COLOR 7
    PRINT "..."
    CHAIN "/programs/rocketsh"
    PRINT "Shell process ended."
UNTIL FALSE
```

- Sets globals
- Mounts extra filesystems
- Launches `rocketsh`
- Loops forever, reloading the shell

---

## 🔚 Full Boot Timeline

```
boot.asm
  ↓
enable_fpu / enable_sse
  ↓
kmain()
  ↓
init() → subsystems (heap, IDT, PCI, FS, etc)
  ↓
filesystem_mount("/", "cd0", "iso9660")
  ↓
init_process()
  ↓
proc_load("/programs/init")
  ↓
/programs/init (BASIC):
  - MOUNT
  - CHAIN "/programs/rocketsh"
  ↓
/programs/rocketsh (shell)
```

---

## ✨ Why This Design Works

Retro Rocket's boot process is:

- **Transparent**: Fully traceable from boot to shell
- **Modular**: Replace shell or init without kernel changes
- **Learnable**: Great for teaching systems programming
- **Retro-powered**: BASIC everywhere - even `init`
