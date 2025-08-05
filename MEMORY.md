# Retro Rocket Memory Model

Retro Rocket handles memory in a way that is both simple for BASIC users and
efficient under the hood. This document explains how memory is divided between
programs, how it grows, and how it is cleaned up when a program ends.

| Area            | Description                                                                 |
|-----------------|-----------------------------------------------------------------------------|
| **Operating System Core** | Kernel, drivers, and system services. This part is always present and not visible to BASIC programs. |
| **BASIC Programs** | Each BASIC program has its **own private heap**, managed by a *buddy allocator*. |
| Program Heap    | - Starts empty<br>- Grows in fixed-size regions (typically 1MB)<br>- Allocations (variables, arrays, strings) come only from this private heap<br>- When the program ends, its heap is destroyed in one call, instantly freeing all program memory and removing any fragmentation |
| Memory Reporting | - `MEMPROGRAM` → shows the program’s current live usage<br>- `MEMPEAK` → shows the program’s highest memory use during this run<br>- `MEMUSED` → shows the whole system's memory usage<br>- `MEMFREE` → shows the whole system's memory free amount |
| Other Subsystems | Certain subsystems (like ACPI during boot) also get their own heaps, which are discarded when no longer needed. BASIC users usually don’t see these. |

---

## Why This Matters

- **Isolation**: Each BASIC program has its own memory pool. Programs cannot interfere with each other’s memory.
- **Speed**: Allocation and free are nearly O(1) operations. Programs can create and destroy variables and arrays extremely quickly.
- **Simplicity**: No long cleanup routines are needed. When a program ends, its heap vanishes in one step.
- **Transparency**: You can measure exactly how much memory your program used at its peak, and how much is live right now, using BASIC functions.
