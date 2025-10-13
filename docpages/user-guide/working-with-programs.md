\page working-with-programs Working with Programs

Retro Rocket is designed around BASIC programs.
Most users will simply run existing programs - either the ones included with the system or new ones you download.

---

### Running programs
Programs are stored under the `/programs` directory.
To run a program from the shell, type its name in lowercase and press **Enter**:

```
about
```

This starts the program `about`.
When it ends, you return to the shell prompt.

From inside a BASIC program, the keyword `CHAIN` can also be used to run another program, but this is mainly for programmers.

---

### Where programs live
- `/programs` — the main directory for programs.
- `/programs/libraries` — shared code used by programs.
- `/programs/drivers` — BASIC drivers used by the system.

If you install Retro Rocket to a hard disk, you can add more programs here.
When running from the LiveCD, you can temporarily place downloaded programs in `/ramdisk`.

---

### Notes
- In LiveCD mode, anything saved in `/ramdisk` is lost on shutdown or reboot.
- On an installed system, programs and files are stored on the hard disk permanently.
