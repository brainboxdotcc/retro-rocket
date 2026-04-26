\page kernel-profiling-osdev Kernel Profiling in Retro Rocket

This page describes how to enable and use the built-in kernel profiler in Retro Rocket. It is intended for OS developers working on the kernel and low-level subsystems.

## Overview

Retro Rocket includes a lightweight profiling system based on GCC’s function instrumentation hooks:

```text
-finstrument-functions
```

When enabled, the compiler injects calls on every function entry and exit. The kernel collects total cycles per function, call counts, and parent to child call relationships.

The output is written in **Callgrind format**, which can be opened in tools such as KCachegrind or QCachegrind.

## Enabling Profiling

Profiling is disabled by default. To enable it, build the kernel with the `PROFILE_KERNEL` CMake flag:

```bash
cmake -DPROFILE_KERNEL=ON -S source-dir -B build-dir
cmake --build build-dir
```

This will include the profiler subsystem in the kernel and significantly slow down execution (this is expected)

## Running with Profiling Enabled

Boot the kernel as usual. Profiling is active immediately.

\warning Profiling introduces heavy overhead. Expect the system to run **2-5× slower**. Timing-sensitive code may behave differently. Only use profiling for analysis, not normal operation.

## Triggering a Profile Dump

Press:

```text
CTRL + ALT + SHIFT + P
```

This will disable interrupts, freeze profiling data, dump the profile to the first serial port, and resume execution.

## Capturing Output (QEMU)

Run QEMU with serial output redirected:

```bash
qemu-system-x86_64 ... -serial file:callgrind.out
```

After triggering a dump, the file `callgrind.out` will contain the profile. Running the dump twice in the same session will append both dumps together.
Doing so is not recommended, as KCachegrind and similar programs are not designed for this and may crash.

If you need multiple profiles, clear or rename the output file between runs.

## Viewing the Profile

Open the output with:

```bash
kcachegrind callgrind.out
```

Further documentation on KCachegrind can be found on the [official website for the project](https://kcachegrind.github.io/html/Documentation.html).

## Limitations

* Profiling is global and always-on when enabled
* High overhead makes real-time behaviour unreliable
* Intended for development, not production use
* The profiler will not and cannot profile itself