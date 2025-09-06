\page tasks Writing BASIC Tasks

A **task** in Retro Rocket is a background process. Unlike a normal *program* that runs in the foreground and owns the console, a *task* runs independently of the shell and continues until explicitly terminated.

Tasks can be used for long-running background services, or even as **device drivers** (for example, the `ps2mouse` task which implements the mouse driver entirely in BASIC).

---

## Starting and stopping tasks

To run a program as a task:
@code
ROCKETSH> task drivers/ps2mouse
@endcode
This loads the program from `/programs/drivers/ps2mouse` and runs it in the background.

Tasks keep running until the system halts or they are killed with the shell’s process control commands.

---

## What tasks are good for

- Handling **device drivers** and **daemon-type activity** (mouse, timers, network servers).
- Providing **background services** (logging, monitoring, caches).
- Responding to **requests from other programs** (e.g. via UDP, shared state, or libraries).
- Anything that needs to run "behind the scenes" while the user works in the shell or another program runs in the foreground.

---

## What not to do in a task

Because tasks run without a console of their own:

- **Do not print to the console** (`PRINT`, `CLS`, `CURSOR`, etc.). This will disrupt whatever foreground program is running.
- **Do not assume interactive input.** Tasks should never call `INKEY$`, `INPUT`, or similar.
- **Avoid blocking calls** that wait forever on user input. Tasks should loop, sleep, or use nonblocking calls appropriately.
- **Keep state contained.** Do not overwrite global variables that other programs might depend on.
- **Be polite.** Use background loops that do real work but do not starve the system by calling busy FNs (e.g. poll at a reasonable interval rather than spinning as fast as possible).

---

## Structure of a typical task

A task is usually structured as:

1. **Initialisation** - set up UDP ports, device state, or memory structures.
2. **Main loop** - perform the background activity (poll hardware, read input, handle requests).
3. **Clean shutdown** - release resources when exiting (e.g. `UDPUNBIND`).

Example skeleton:

@code
DEF PROCmain
REM initialisation
UDPBIND "127.0.0.1", 13802

REPEAT
REM background work
PROCpoll_device
PROChandle_requests
UNTIL FALSE
ENDPROC
@endcode

---

## Notes

- Tasks share the same BASIC runtime as other programs, so they must be **well-behaved** and not interfere with the console.
- Use **libraries** to expose a clean API to foreground programs (for example, the `mouse` library calls into the `ps2mouse` task).
- Remember that tasks continue running after the shell returns - always consider resource cleanup (`PROCdone`) where applicable.