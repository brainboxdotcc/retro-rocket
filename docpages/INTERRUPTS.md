# Interrupt Handling in Retro Rocket (AMD64)

Retro Rocket uses a modern, APIC-based interrupt system suitable for 64-bit long mode. It fully supports Symmetric Multiprocessing (SMP) and I/O APICs via ACPI detection, with legacy PIC support deprecated and disabled after boot.

---

## Overview

Interrupts in Retro Rocket are composed of three tightly integrated layers:

| Layer      | Component               | Role                                 |
|------------|-------------------------|--------------------------------------|
| IDT        | `idt.c`                 | Sets up the Interrupt Descriptor Table (IDT) and traps |
| APIC       | `apic.c`                | Local APIC timer, EOI, spurious interrupt handling |
| ACPI / MADT| `acpi.c`                | Enumerates Local APICs, IOAPICs, IRQ/GSI mappings |

---

## IDT Initialisation (`idt.c`)

Retro Rocket sets up a full 256-entry Interrupt Descriptor Table, aligned on a 16-byte boundary. The `init_idt()` function:

1. Clears all entries to a known zero state.
2. Registers `IRQ0` (timer) as a demonstration.
3. Calls `idt_init()` (defined in `loader.S`) to populate handler stubs.
4. Loads the IDT via `lidtq`.

The legacy PIC is briefly remapped to avoid IRQ overlaps with CPU exceptions, and then **disabled entirely** if APIC is enabled.

```c
__asm__ volatile("lidtq (%0)" :: "r"(&idt64));
```

### All Interrupts Are Initially Masked

Each IRQ is masked until a handler is explicitly registered using:

```c
register_interrupt_handler(irqnum, handler, dev, ctx);
```

This design guarantees no spurious IRQs during boot and enforces strict control of enabled sources.

---

## APIC Handling (`apic.c`)

Retro Rocket assumes all hardware conforms to the AMD64 specification. This includes mandatory support for:

- **Local APIC (LAPIC)**: Per-core interrupt controller for inter-processor signalling and timers.
- **I/O APIC (IOAPIC)**: Routes external device interrupts to LAPIC via GSIs (Global System Interrupts).

### Key APIC Features

- **LAPIC Base Detection**: Via the ACPI MADT.
- **Timer Configuration**: One-shot mode, calibrated via TSC.
- **End-of-Interrupt (EOI)**: Sent with `apic_write(APIC_EOI, 0)` after each interrupt.
- **Spurious IRQ Handling**: IRQ 7 is handled explicitly and does not send EOI.

---

## ACPI & MADT Processing (`acpi.c`)

The ACPI module is responsible for:

- Initialising `uACPI` and parsing the RSDP, RSDT, and MADT tables.
- Discovering:
  - Local APIC base address (32- or 64-bit form)
  - All LAPIC IDs (active CPUs)
  - All IOAPICs and their GSI ranges
  - Interrupt overrides (e.g. legacy IRQ remapping)

### IRQ to GSI Routing

The system maintains a `pci_irq_routes[]` map of IRQ→GSI mappings with polarity and trigger mode.

Interrupt sources are discovered from:

1. **MADT entries** (APIC table)
2. **ACPI `_PRT`** entries (`EXT_IRQ` and `IRQ` resources)
3. Fallback: direct 1:1 IRQ to GSI mapping

This routing is queried using:

```c
uint32_t irq_to_gsi(uint8_t irq);
uint8_t get_irq_polarity(uint8_t irq);
uint8_t get_irq_trigger_mode(uint8_t irq);
```

---

## Registering a Handler

Drivers register handlers with:

```c
register_interrupt_handler(uint8_t irq, handler_t fn, device_t dev, void *ctx);
```

This:

- Unmasks the interrupt line (GSI) in the APIC.
- Enables delivery from the specified source.
- Ensures the handler will be called on interrupt.

If not registered, the line remains masked and ignored.

---

## Legacy PIC Support (Deprecated)

- Retro Rocket targets only **AMD64 hardware**. There is no support for 32-bit x86.
- The legacy PIC is only touched briefly during boot for compatibility and **immediately disabled** once APIC initialisation completes.
- All AMD64 systems (including virtualised environments) must support LAPIC and IOAPIC per specification.

**TL;DR:** APIC is mandatory.

---

## Terminology

| Term         | Meaning                                       |
|--------------|-----------------------------------------------|
| IRQ          | Legacy interrupt request (ISA, PIC)          |
| GSI          | Global System Interrupt (IOAPIC input)        |
| LAPIC        | Local Advanced Programmable Interrupt Ctrl.   |
| IOAPIC       | I/O Advanced Programmable Interrupt Ctrl.     |
| MADT         | Multiple APIC Description Table (ACPI)        |
| PIC          | Legacy 8259 Programmable Interrupt Controller |
