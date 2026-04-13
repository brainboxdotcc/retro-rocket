\page interrupts Interrupt Handling in Retro Rocket (AMD64)

[TOC]

Retro Rocket uses a modern, APIC-based interrupt system suitable for 64-bit long mode. It fully supports Symmetric Multiprocessing (SMP) and I/O APICs via ACPI detection, with legacy PIC support deprecated and disabled after boot.

Retro Rocket supports two interrupt delivery mechanisms:

* **IDT**: traditional x86 interrupt delivery via the Interrupt Descriptor Table
* **FRED**: Flexible Return and Event Delivery, using MSR-configured entry points and hardware-defined event frames

At boot, the kernel attempts to initialise FRED first. If FRED is unavailable or disabled, it falls back to the IDT path.

---

## Overview

Interrupts in Retro Rocket are composed of four tightly integrated layers:

| Layer     | Component         | Role                                                  |
|-----------|-------------------|-------------------------------------------------------|
| Delivery  | `idt.c`, `fred.c` | Sets up the CPU interrupt delivery mechanism          |
| Routing   | `apic.c`          | Local APIC timer, EOI, spurious interrupt handling    |
| Discovery | `acpi.c`          | Enumerates Local APICs, IOAPICs, IRQ/GSI mappings     |
| Dispatch  | `interrupt.c`     | Dispatches exceptions and IRQs to registered handlers |

The delivery mechanism determines how the CPU enters the kernel on an interrupt or exception. The routing layer determines how external interrupt sources reach the CPU. Both delivery mechanisms ultimately dispatch into the same higher-level interrupt handling code.

---

## Interrupt Delivery Mechanisms

Retro Rocket supports two interrupt delivery mechanisms on AMD64:

| Mechanism | Component | Role                                                   |
|-----------|-----------|--------------------------------------------------------|
| IDT       | `idt.c`   | Traditional descriptor-table-based interrupt delivery  |
| FRED      | `fred.c`  | Modern MSR-configured interrupt and exception delivery |

The kernel attempts to enable FRED first:

```cpp
if (fred_enabled && init_fred()) {
```

If this fails, it falls back to IDT initialisation.

### Shared BSP Interrupt Setup

Both delivery mechanisms share common BSP setup for interrupt handling. This includes:

1. Early interrupt subsystem initialisation
2. PIT programming
3. IRQ routing setup
4. Final interrupt enablement

This avoids duplicating platform interrupt setup while allowing the CPU entry mechanism to differ.

### Shared High-Level Dispatch

Both IDT and FRED ultimately dispatch into the same higher-level interrupt handling functions in `interrupt.c`:

* `Interrupt()` for CPU exceptions and software interrupts
* `IRQ()` for hardware interrupt delivery

This keeps IRQ routing, handler registration, entropy collection, FPU state save/restore, and end-of-interrupt handling consistent regardless of delivery mechanism.

---

## IDT Initialisation (`idt.c`)

Retro Rocket sets up a full 256-entry Interrupt Descriptor Table, aligned on a 16-byte boundary. The `init_idt()` function:

1. Clears all entries to a known zero state.
2. Registers `IRQ0` (timer) as a demonstration.
3. Calls `idt_init()` (defined in `loader.S`) to populate handler stubs.
4. Loads the IDT via `lidtq`.

The legacy PIC is briefly remapped to avoid IRQ overlaps with CPU exceptions, and then **disabled entirely** if APIC is enabled.

```cpp
__asm__ volatile("lidtq (%0)" :: "r"(&idt64));
```

### All Interrupts Are Initially Masked

Each IRQ is masked until a handler is explicitly registered using:

```cpp
register_interrupt_handler(irqnum, handler, dev, ctx);
```

This design guarantees no spurious IRQs during boot and enforces strict control of enabled sources.

---

## FRED Initialisation (`fred.c`)

Retro Rocket can use Intel FRED as an alternative to IDT-based interrupt delivery on supported CPUs.

FRED uses:

* MSR-configured entry points instead of IDT gates
* Hardware-defined event frames instead of the traditional interrupt stack frame
* `erets` instead of `iretq` on return from the low-level entry path

The `init_fred()` function performs the same BSP interrupt setup as the IDT path, then enables FRED for the current CPU and completes late interrupt initialisation.

FRED support is detected via CPUID before being enabled. FRED is enabled per CPU, and application processors enable it separately during AP startup.

### Per-CPU Enablement

FRED state is not global. Each CPU must enable it independently. The BSP does this during `init_fred()`, and APs do so during shared interrupt loading.

### FRED Entry Stubs (`loader.S`)

The low-level FRED entry stubs live in `loader.S`.

Retro Rocket provides:

* a ring 3 stub at the start of the configured FRED entry page
* a ring 0 stub at the required offset within that same page

The ring 0 entry stub saves general-purpose registers, passes a pointer to the hardware-constructed FRED frame into C, and returns using `erets`.

The FRED entry page must be laid out exactly as required by the architecture. The configured entry address must point at the start of the page containing the stubs.

### FRED Frame Handling

On entry, FRED provides a hardware-defined event frame containing saved architectural state and event metadata. Retro Rocket decodes the delivered vector from the FRED frame and dispatches either to `Interrupt()` or `IRQ()`.

This allows FRED to reuse the same higher-level interrupt and exception handling logic as the IDT path.

---

## Assembly Stubs (`loader.S`)

The low-level interrupt and exception entry stubs are implemented in `loader.S`.

For the IDT path, `loader.S` provides:

* the code that populates the IDT entries
* exception stubs for CPU exceptions
* IRQ stubs for hardware interrupt vectors
* common dispatch trampolines for exceptions and IRQs

These stubs save register state, prepare arguments for the C dispatch layer, call into `Interrupt()` or `IRQ()`, restore register state, and return with `iretq`.

For the FRED path, `loader.S` also provides the FRED entry-page stubs used by `fred.c`.

This means `loader.S` contains the low-level entry machinery for both interrupt delivery mechanisms.

---

## APIC Handling (`apic.c`)

Retro Rocket assumes all hardware conforms to the AMD64 specification. This includes mandatory support for:

* **Local APIC (LAPIC)**: Per-core interrupt controller for inter-processor signalling and timers.
* **I/O APIC (IOAPIC)**: Routes external device interrupts to LAPIC via GSIs (Global System Interrupts).

### Key APIC Features

* **LAPIC Base Detection**: Via the ACPI MADT.
* **Timer Configuration**: One-shot mode, calibrated via TSC.
* **End-of-Interrupt (EOI)**: Sent with `apic_write(APIC_EOI, 0)` after each interrupt.
* **Spurious IRQ Handling**: IRQ 7 is handled explicitly and does not send EOI.

APIC is responsible for interrupt routing and delivery to a CPU. It is not responsible for the CPU-side interrupt entry mechanism itself. That role is handled by either the IDT or FRED.

---

## ACPI & MADT Processing (`acpi.c`)

The ACPI module is responsible for:

* Initialising `uACPI` and parsing the RSDP, RSDT, and MADT tables.
* Discovering:

  * Local APIC base address (32- or 64-bit form)
  * All LAPIC IDs (active CPUs)
  * All IOAPICs and their GSI ranges
  * Interrupt overrides (e.g. legacy IRQ remapping)

### IRQ to GSI Routing

The system maintains a `pci_irq_routes[]` map of IRQ→GSI mappings with polarity and trigger mode.

Interrupt sources are discovered from:

1. **MADT entries** (APIC table)
2. **ACPI `_PRT`** entries (`EXT_IRQ` and `IRQ` resources)
3. Fallback: direct 1:1 IRQ to GSI mapping

This routing is queried using:

```cpp
uint32_t irq_to_gsi(uint8_t irq);
uint8_t get_irq_polarity(uint8_t irq);
```

---

## Registering a Handler

Drivers register handlers with:

```cpp
register_interrupt_handler(uint8_t irq, handler_t fn, device_t dev, void *ctx);
```

This:

* Unmasks the interrupt line (GSI) in the APIC.
* Enables delivery from the specified source.
* Ensures the handler will be called on interrupt.

If not registered, the line remains masked and ignored.

Handlers are ultimately invoked by the common dispatch layer in `interrupt.c`, regardless of whether the interrupt reached the kernel through IDT or FRED.

---

## Legacy PIC Support (Deprecated)

* Retro Rocket targets only **AMD64 hardware**. There is no support for 32-bit x86.
* The legacy PIC is only touched briefly during boot for compatibility and **immediately disabled** once APIC initialisation completes.
* All AMD64 systems (including virtualised environments) must support LAPIC and IOAPIC per specification.

**TL;DR:** APIC is mandatory.

---

## Terminology

| Term   | Meaning                                       |
| ------ | --------------------------------------------- |
| IRQ    | Legacy interrupt request (ISA, PIC)           |
| GSI    | Global System Interrupt (IOAPIC input)        |
| LAPIC  | Local Advanced Programmable Interrupt Ctrl.   |
| IOAPIC | I/O Advanced Programmable Interrupt Ctrl.     |
| MADT   | Multiple APIC Description Table (ACPI)        |
| PIC    | Legacy 8259 Programmable Interrupt Controller |
| IDT    | Interrupt Descriptor Table                    |
| FRED   | Flexible Return and Event Delivery            |
| ERETS  | FRED return instruction                       |
