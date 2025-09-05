# Retro Rocket BASIC

*A curious developer’s guide to how the interpreter works and why it’s built this way.*

This readme is aimed at C developers and language tinkerers. It explains the design of Retro Rocket BASIC: how it parses and executes programs, how types behave, how control flow is implemented, and why the runtime is cooperative rather than preemptive. It’s intentionally narrative and stable over time.

---

## Why this interpreter exists

Retro Rocket BASIC is meant to be **small, predictable, and easy to reason about**. It lives inside a kernel-ish environment and talks directly to the host (console, files, sockets, graphics). The priority is **clarity over machinery**:

* No AST, no bytecode, no JIT.
* No threads inside the interpreter.
* Strong, suffix-based types (`$` strings, `#` reals, no suffix = integers).
* Line-numbered source and direct jumps.

If you like stepping through code with a debugger and understanding exactly why a line does what it does, you’re the target audience.

---

## Big picture

The program is kept as a single text buffer of numbered lines. The tokenizer reads directly from that buffer and yields tokens. Each line is executed by a **statement dispatcher** that switches on the current token and performs the action immediately. Expressions are evaluated by a **hand-written precedence parser** that returns a *typed* value (int, real, or string). Control flow is implemented by moving pointers back and forth in the original text rather than running bytecode.

That single-pass feel is the core of the design: **parse just enough to do the work now**, then move the pointer and carry on.

---

## Parsing model: hand-written, no AST

The interpreter uses a hand-written tokenizer and a recursive-descent expression parser. There is **no parse tree** and **no intermediate representation**. The advantages are practical:

* The working set stays tiny (no trees to allocate or free).
* Debugging is direct: the call stack shows exactly which grammar layer you’re in.
* There’s nothing to “compile,” so startup is immediate.

The trade-off is that you won’t get whole-program optimizations or structural rewrites. In this project, that’s a feature: the runtime’s behavior is **obvious**, not clever.

If you’ve used uBASIC/TinyBASIC, this will feel familiar, but with strict typing and a more complete expression grammar.

---

## Line numbering, auto-number, and execution

Retro Rocket BASIC **encourages (and for libraries, requires)** source files **without** explicit line numbers. On load, a pre-pass (`auto_number`) scans the raw text and **synthesizes monotonically increasing line numbers** (e.g., 10, 20, 30, …) into the in-memory buffer. From that point on, the interpreter treats the program as numbered.

Why this way?

* **Human-friendly source.** You write and diff clean, unnumbered files.
* **Library safety.** Unnumbered libraries avoid accidental number collisions across modules.
* **Deterministic runtime.** Internally assigned numbers give stable anchors for jumps, block matching, `ON ERROR`, and diagnostics (`ERRLINE`).

> **Deprecation note:** `GOTO` and `GOSUB` are supported for legacy programs but **deprecated**. Prefer **`PROC`/`FN`** calls (with `ENDPROC` / `=` return) for structured control flow. Likewise, `ON ERROR GOTO/GOSUB` is deprecated in favor of **`ON ERROR PROC`**.

If you insist on writing explicit numbers, the tokenizer will accept them, but it’s discouraged unless you went to school with a T-Rex.

Under the hood, execution is unchanged:

* At startup, the interpreter builds a fast map from **line number → pointer into the text buffer**.
* Control transfers (legacy `GOTO`/`GOSUB` and structured `PROC`/`FN`) reset the tokenizer pointers to the target location. There is **no VM program counter**; the “PC” is literally a pointer into the program text.

Two practical consequences:

* **Jumps are O(1)**: a table lookup and a pointer assignment.
* **Block matching** (e.g., finding the `ELSE` for an `IF`) uses a small scan with a depth counter-simple and robust.

---

## The expression engine: typed, predictable

Expressions are parsed with classic precedence: factors → unary ± → `* / MOD` → `+ -` (also concatenation) → relations → boolean `NOT/AND/OR`.

Key rules:

* **Types are real.** The engine returns one of three concrete kinds: INT (64-bit), REAL (double), or STR (char\*).
* **Promotion is one-way.** INT may promote to REAL if needed; nothing else auto-converts.
* **No string↔number guessing.** `"12" + 3` is an error, not a surprise. `STR + STR` concatenates; otherwise `+` is numeric.
* **Comparisons are typed.** Strings compare lexically; numbers compare numerically. The result is an integer truth value (0/1).
* **Boolean ops are straightforward.** `NOT`, `AND`, `OR` operate on truthiness (non-zero / non-empty). The engine keeps evaluation simple and side-effect-free.

The design goal is that you never wonder *what* will happen; you only choose *which* type you want by using (or omitting) a suffix.

---

## Names, keywords, and performance hints

Keywords are kept in alphabetical order so the tokenizer can bail early when scanning. Variables are conventional BASIC names with optional `$` or `#` suffixes. The interpreter stores *three* symbol lists, one per type, and one stack of “locals” per call depth for each type.

A small optimization matters here: **move-to-front** on lookup hits. Hot names (loop counters, accumulators, common strings) migrate to the head of the list, which keeps lookup costs low without pulling in a full hash table.

---

### Memory & allocator model

Retro Rocket BASIC keeps memory management **deterministic** on purpose. There’s no tracing GC, no background compaction, and no “surprise” pauses. Instead, the interpreter uses a small set of arenas backed by a buddy allocator and ties lifetime directly to program structure (globals vs. call frames).

At the top level, a BASIC program runs inside a single interpreter context that owns a **global arena**. Anything that should live for the duration of the program. Global variables, the pre-parsed `DEF PROC/FN` table, the line index, and other long-lived structures reside here. Strings are always **copied into interpreter ownership** when they enter the runtime: built-ins that return text use an internal `gc_strdup` equivalent so the caller never receives a pointer to transient storage. Reassigning a string variable transparently frees the old value and stores a new owned copy; the same rule holds for string array elements when you overwrite a slot. You don’t free these yourself. Ownership is the interpreter’s.

When you enter a `PROC` or `FN`, the interpreter creates a **local frame**: three per-type stores (int/real/string) and their associated allocations become local to that call depth. Argument binding, locals created inside the routine, and any strings produced while evaluating those locals are allocated against this frame. On `ENDPROC`/`RETURN`, the frame is dismantled in one shot (`free_local_heap`), and all memory tied to it is released en bloc. This frame model makes lifetime obvious: globals exist until program end; locals exist until the procedure/function returns. As a corollary, never cache raw pointers to interpreter-owned strings across a frame boundary, and don’t cache them across assignments to the same symbol either. An assignment may replace the storage underneath you.

Function return values follow the same discipline without leaking. Internally, `FN` bodies run to a `=` return where the interpreter captures the value in the callee’s context; the caller then **duplicates** the result into its own context before the callee frame is torn down. That’s why you can safely use the returned string after the function exits: the caller owns its copy.

Arrays are “owned the same way” as scalars: int arrays, real arrays, and string arrays each live in the appropriate arena (global or the current local frame). Indexing performs bounds checks; replacing an element updates ownership correctly (for strings, the previous element’s buffer is freed before the new value is copied in). List-like `PUSH`/`POP` operations are implemented over contiguous storage; they may move elements but they don’t change who owns the element memory. If you push a string, the runtime first takes a copy and then manages it.

Two implementation details are worth calling out for people extending the runtime. First, **move-to-front** lists are used for variable lookups inside each store: every successful lookup bubbles that node to the head, which keeps hot variables effectively O(1). This is an algorithmic optimization, not a lifetime mechanism; nodes still belong to whichever arena created them. Second, graphics resources (e.g., sprite pixel buffers loaded via `stb_image`) are deliberately **not** allocated in the BASIC string arenas. They use the kernel’s allocator under the hood and are freed explicitly when you call `SPRITEFREE` or when the interpreter is torn down. Treat them like OS resources: allocate, use, free.

From an extension author’s perspective, the rules are simple and strict: whenever you return text to BASIC, **hand back interpreter-owned memory** (duplicate it into the current context); never return a pointer into a stack buffer, a static scratch area, or an OS-owned buffer. If you allocate temporary buffers to talk to the kernel (read a file, receive from a socket, decode an image), either free them immediately after you’ve copied results into interpreter storage, or attach them to a resource with a clear destruction path. This model               global arena for the program, frame arenas for calls, and explicit duplication at boundaries, keeps performance predictable and makes memory errors easy to localize.

---

## Program lifecycle & scheduler handshake

Retro Rocket BASIC is driven **one line at a time**. The host scheduler repeatedly invokes the interpreter’s “run step”; each call is expected to execute **exactly one atomic line** parse the line label, execute one statement, and return or to *yield* early if the statement can’t complete yet. This simple contract gives you **language-level pre-emption** without threads: the OS interleaves BASIC with other work by round-robin calling the run step on runnable processes.

**Startup.** When a program is loaded, the source is **auto-numbered** if it doesn’t already carry line numbers (this is encouraged for programs and **required for libraries**). A fast index from line number to position in the text buffer is built, and procedure/function headers are recorded with their names, parameters, and defining lines. System variables (PID, screen geometry, `PI#`, `E#`, vendor strings, …) are seeded into the initial context.

**Run step.** On each scheduler tick, the interpreter:

1. Reads the current line label and dispatches the single statement on that line.
2. If the statement completes, control naturally advances to the next line and returns to the host **one line per call**.
3. If the statement would block (waiting for user input, socket data, a sleep interval, etc.), it **registers an idle predicate**, rewinds its internal cursor to the same line, marks itself not runnable, and returns. The host calls back later only when the predicate says it’s ready, at which point the statement resumes and finishes.

**Cooperative waits.** Several built-ins follow the same yield-and-resume pattern:

* **Input**: if a complete line of text isn’t available, yield until it is; then read, assign, and finish.
* **Socket reads**: if no data is pending, yield until the socket becomes readable; then decode to string/real/int by suffix and finish.
* **Sleep**: record a wakeup time, yield until the clock passes it, then continue.

This shape keeps the interpreter single-threaded and re-entrancy-free while still cooperating tightly with the OS. Long CPU loops still execute within a single line, but structured control flow naturally spans multiple lines (`FOR…NEXT`, `WHILE…ENDWHILE`, `REPEAT…UNTIL`), so control returns to the scheduler between each step. There’s also an explicit `YIELD` for voluntary hand-off.

**Why it matters.** Driving execution as “one atomic line per call, or yield” gives deterministic scheduling, eliminates hidden blocking, and makes embedding trivial: no threads to manage, no timeslices to tune, just call the run step again when the process is runnable.

---

## Procedures and functions

User code can define `PROC`edures and `FN`s. A quick pre-pass indexes their names, parameter lists, and line numbers. Calls then:

1. Evaluate actual arguments.
2. Create a fresh local frame (per type).
3. Bind arguments by name.
4. Push a return line number.
5. Jump to the definition.

Functions return with `=` inside the body (the function’s suffix decides the return type). Procedures end with `ENDPROC`. This keeps call mechanics obvious and cheap.

---

## Control flow without a compiler

Structured constructs (`IF/ELSE/ENDIF`, `WHILE/ENDWHILE`, `REPEAT/UNTIL`) are implemented by scanning the token stream with a small depth counter to find the matching close token, then jumping to the recorded line number. Unstructured `GOTO/GOSUB/RETURN` just use the line map.

This keeps the implementation lightweight while preserving the constructs people expect.

---

## Cooperative runtime: statements yield

The runtime is **cooperative**. Any statement that would block, waiting for a keypress, a socket, or a timer, **yields**:

* It registers a small *idle check* callback (e.g., “is there a key?” “did the deadline pass?”).
* It marks the process as IO-bound or suspended.
* It jumps to the *current* line so that, when scheduled again, the **same statement** will re-run from the top with normal parsing.

When the callback says “ready,” the scheduler resumes the interpreter, the statement runs, and execution proceeds.

This model keeps the interpreter single-threaded and avoids re-entrancy. You never find yourself halfway through an expression when an interrupt fires. The cost is that long, tight loops *must* be polite (or call a yielding primitive) or they will hog the CPU. In practice, BASIC programs that do I/O already pass through yielding statements.

---

## Error handling you can script

All errors go through one place. The interpreter sets:

* `ERR = 1` (auto-clears the next time you read it),
* `ERR$ = "message"` (sticky until overwritten),
* `ERRLINE = <line number>`.

Optional `ON ERROR PROC name` routes control to user code. The interpreter:

* Skips to the end of the current line (so partial statements don’t execute twice),
* Records the *line after* the error as the return point,
* Pushes a fresh local frame and jumps to the named handler.

The important bit is that the interpreter never prints behind your back; user code decides what to do with error state.

### Ctrl+Esc (“Escape”) semantics

Retro Rocket BASIC treats **Ctrl+Esc** as a **user-initiated error**, exactly like pressing **ESC** on a BBC Micro. It is **not** a silent wake-up: it **bubbles up to the main run loop**, which emits the error **“Escape”** through the normal tokenizer error path.

**What the interpreter does**

* Sets the standard error variables:

  * `ERR ← 1` (auto-clears the next time you read `ERR`)
  * `ERR$ ← "Escape"` (**sticky** until explicitly cleared to `""`)
  * `ERRLINE ← <current line number>`
* If an `ON ERROR PROC handler` is installed:

  * The interpreter skips to the end of the current line, pushes the return point (the line **after** the error), and **jumps to the handler** with a fresh local frame.
  * When the handler `ENDPROC`s, execution resumes at the saved return line.
* If no handler is installed:

  * The error is printed and the program terminates (consistent with other runtime errors).

**How the signal is detected**

* The keystroke sets an internal **escape latch** (`basic_esc()`).
* Long-latency statements (e.g., `INPUT`, `SLEEP`, `SOCKREAD`) poll this latch via their **idle callbacks**. When asserted, the idle callback stops waiting, control returns to the run loop, and the run loop converts the latch into the `"Escape"` error.
* Between ordinary statements, the run loop also checks the latch, so tight loops without blocking will still trap at the **next statement boundary**.

**Design intent**

* Provides a **deterministic, non-reentrant** way to abort waits and loops. No async signals, no partial state mutations from random contexts.
* Keeps all escape handling on the **single, well-defined error path**, so `ON ERROR PROC` can uniformly intercept it.
* Mirrors the **BBC Micro** interaction model developers expect.

**Contributor guidelines**

* Any new blocking/idle statement must **poll `basic_esc()`** in its wait predicate so Ctrl+Esc can be honored promptly.
* Do **not** swallow or “handle” the escape inside a built-in; let it bubble to the run loop so `ERR/ERR$/ERRLINE` and `ON ERROR PROC` behave consistently.
* Remember: `ERR$` is **sticky**. Once set non-empty it won’t be overwritten by subsequent errors until explicitly cleared to the empty string.

---

### Arrays & list-like operations

Arrays in Retro Rocket BASIC are deliberately **simple, typed vectors**. One dimension, fixed length, and the same `$/#/int` suffix rules as scalars. You create them with `DIM`, you can change their length with `REDIM`, and you index them with an integer expression. That’s it. No hidden growth, no iterators, no surprises.

An array’s **type is part of its name**: `A()` holds integers, `R#()` holds reals, `S$()` holds strings. Index expressions are evaluated by the expression engine and treated as integers; out-of-range indices raise a normal runtime error (which you can catch with `ON ERROR PROC …`). Because arrays are first-class variables, they obey the same **scope and lifetime** rules: locals live for the duration of the current `PROC`/`FN` frame; globals persist for the program’s life.

`DIM` allocates a contiguous block sized for the declared length. `REDIM` replaces that storage with a new block for the new length; elements that fit in the new size are retained, anything beyond the new end is discarded, and newly created slots are reset to the type’s default (numeric zero or empty string). The intent is **predictable memory and time**: you pay for resizing at the moment you ask for it, not piecemeal while your program runs.

Mutation is direct (`arr(i) = value`) and **stable**. Indexes mean fixed positions. For list-like workflows there are two convenience operations:

* **`PUSH`** inserts at a position by shifting the tail one step to the right and opening a hole at the insertion index. The caller then assigns into that slot.
* **`POP`** removes an element and compacts the tail left by one; the last slot is cleared to the type’s default.

Both are intentionally **O(n) moves** on the single backing array. There’s no hidden capacity or amortized growth; if you need more room, say so with `REDIM`. This keeps execution time obvious and avoids allocator churn mid-loop. For string arrays, each slot owns its buffer: compaction frees/retains those buffers as needed so you don’t leak when popping or churn needlessly when pushing.

Design-wise, arrays are kept this spartan for three reasons. First, **determinism**: fixed size and explicit compaction make worst-case costs obvious when you’re inside a tight frame that must finish before yielding back to the scheduler. Second, **memory hygiene**: one contiguous block per array (plus per-element string buffers) reduces fragmentation and makes lifetime clear. Locals are torn down with their frame, globals only when the program ends. Third, **ergonomics without magic**: `PUSH`/`POP` cover the common “insert/remove while preserving order” use case without inventing a second container type or background reallocation policy.

If you’re writing a library on top of this model, the guidance is simple: choose array lengths deliberately, `REDIM` at known boundaries (not per-iteration), prefer `POP`/`PUSH` near the end of the array when you can minimize movement, and remember that string slots are real ownership boundaries, not just views.

---

## Reflection and scope in one paragraph

The interpreter lets code ask for or set variables by name (`GETVAR*`, `SETVAR*`, `EXISTSVAR*`). This is implemented atop the same per-type symbol lists used for normal access. `LOCAL` and `GLOBAL` on assignments pick the destination list; lookups search the local stack from the current frame outward and then globals. On a hit, the node is moved to the front for locality.

It’s a tiny model, but it covers parameters, locals, globals, and reflective tools consistently.

---

## Graphics as a case study in philosophy

The graphics path shows the general approach: a **simple fast path** for the common case and a **precise slow path** for the interesting one.

* Axis-aligned blits use fixed-point stepping.
* Arbitrary quads use a homography and rasterize with incremental numerators (perspective-correct).
* Only fully opaque source pixels are drawn (avoids blending complexity).
* Auto-flip or manual `FLIP` is available; pick control or convenience.

This is the theme throughout: keep the core obvious, add precision where it buys visible quality, and skip frameworks that add weight.

---

## How this differs from other approaches

* **Compared to AST/bytecode VMs (Lua, MicroPython):**
  This interpreter prioritizes *traceability* over optimization. There’s no IR, no VM, and no GC in the hot path. You pay with fewer global optimizations, but you gain a debugger-friendly, step-through-the-grammar experience and a much smaller surface area.

* **Compared to parser generators (YACC/Bison):**
  The grammar is encoded in C functions, not in a declarative file. That makes small changes fast and tool-free, at the cost of less automatic validation.

* **Compared to classic TinyBASIC/uBASIC:**
  You get strict types by suffix, locals/parameters, a broader expression grammar, cooperative I/O, and a consistent error model.

---

## What this design buys you

* **Mechanical sympathy.** Jumps are pointer rewrites; hot names stay hot; the memory model is simple.
* **Determinism.** No hidden threads, no re-entrancy, no surprise coercions.
* **Approachable internals.** Adding a statement or a built-in is editing a switch and a table, not rebuilding a compiler.

And yes, there are boundaries:

* No whole-program optimizations or constant folding across lines.
* Boolean operators don’t short-circuit.
* If you write a compute-heavy loop that never touches a yielding primitive, you can hog the CPU. (By design: the runtime won’t preempt you.)

---

## If you want to extend it

The safest way to think about extensions is:

1. **Does this block?** If yes, add an idle check and yield; resume the statement cleanly when ready.
2. **What type does it return?** Register in the correct built-in table (int, real, or string) and obey the expression engine’s rules.
3. **Where does it live?** Keep state in the host subsystem; the interpreter should pass values in and out, not own resources it can’t manage.

Follow those three rules and your feature will feel native to the interpreter.

---

*Retro Rocket BASIC chooses *clarity over cleverness*. It’s a line-oriented, typed, cooperative interpreter whose behavior is easy to predict and easy to step through. If you enjoy understanding the system you’re using, and bending it a little, this design is intentionally friendly to you.*