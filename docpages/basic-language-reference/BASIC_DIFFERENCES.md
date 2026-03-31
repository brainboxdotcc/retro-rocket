\page basic-differences Differences Between Retro Rocket BASIC and other BASIC dialects

This page highlights the key differences between Retro Rocket BASIC and traditional BASIC dialects.

### Line numbers are optional

Most classic BASIC systems require line numbers. Retro Rocket BASIC does not. If a program begins with a digit (`0`–`9`), it is treated as numbered; otherwise it is not. When used, line numbers must increase but do not need to be consecutive.

### Plain text storage

Programs are stored as plain text rather than tokenised. They can be edited outside the system, and language changes do not affect existing files.

### Multitasking behaviour

Retro Rocket BASIC runs within a multitasking system where scheduling occurs between BASIC lines.

A program executes one line at a time, and task switching only happens when a line completes. A single line, including any `FN` calls it contains, runs to completion without interruption.

### Multitasking behaviour

Retro Rocket BASIC runs within a multitasking system where scheduling occurs between BASIC lines.

A program executes one line at a time, and task switching only happens when a line completes. A single line, including any `FN` calls it contains, runs to completion without interruption.

### Programs can launch other programs with shared state

Using `CHAIN`, one program can run another and pass variables along. This allows programs to be composed from smaller parts, rather than everything needing to live in a single file or restart from scratch.

### No dependence on immediate mode editing

Many classic BASIC systems were tightly coupled to an interactive line editor. Retro Rocket BASIC treats programs as files first, with editing and execution as separate steps. This makes it behave more like
a conventional development environment while still keeping the simplicity of BASIC.

### Libraries are built-in

Code reuse is part of the language rather than an afterthought. The `LIBRARY` statement allows programs to load shared modules at runtime, so larger projects can be split into multiple files instead of
being a single monolithic program.


