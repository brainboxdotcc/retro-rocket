\page rocketsh rocketsh Command (Shell)

`rocketsh` is the interactive shell used to execute commands and run programs.
It is launched by `init` at boot and is always available at the system prompt.

\image html rocketsh.png

This page describes `rocketsh` in detail. For a simple overview of using the shell, see @ref desktop-and-shell "The Shell" in the User Guide.

### Command line entry

The `rocketsh` shell supports command line editing. You may use the cursor `LEFT` and `RIGHT` keys plus `HOME` and `END` to navigate in the current line and edit it, inserting into the line if neccessary.

Alternately, you can use the cursor `UP` and `DOWN` keys to move through the history of recently entered commands. If you edit one of these commands or send it, it will be inserted into the history again as the latest command.

The history buffer will grow as needed to accomodate as many history items as you enter.

Pressing `ESC` will abort the current line entry, clearing its contents and advancing down to the line below.

These interactive line editing facilities are also available to any other BASIC programs via the `ansi` library.

### Rocket Shell Commands

The `rocketsh` shell supports two forms of input; firstly any command you type is searched for as a program under the `/programs` directory. If it can be found it is executed. The path under which `rocketsh` searches for programs to run can be changed, as shown below.

If no matching program can be found, the line is passed to \ref EVAL "EVAL" instead and any changes to the program state will be inherited by `rocketsh`, including documented variables below.

### BASIC execution

Any line entered at the prompt which is not resolved to a program is executed using \ref EVAL "EVAL".

This means the shell accepts full BASIC syntax directly at the prompt, including:

- Variable assignment
- Expressions
- Procedure and function calls
- Control flow (via multi-line entry)

All built-in BASIC keywords, operators, and functions are available exactly as they are within a BASIC program.

```basic
A = 10
PRINT A * 2
PROCdemo
```

Single-line input executes in the current shell context.

Multi-line input (see below) executes as an anonymous child program.

Changes made by single-line execution persist in the shell environment.

### Multi-line program entry

`rocketsh` supports entering and executing multi-line BASIC programs directly at the prompt.

- Enter `[` on a new line to begin multi-line entry mode.
- The prompt will change to numbered lines (**1**, **2**, **3**, ...).
- Enter BASIC statements line by line.
- Enter `]` on a new line to execute the program.
- To abort entering a multi-line program without running it, press ESC
- You can use cursor keys to edit and recall previous lines
- Each line you enter becomes part of the rocketsh command history

\image html repl-entry.png

The entered lines are combined and executed using \ref EVAL "EVAL" as an anonymous program.

### Built-in commands

#### cd / chdir

Change the current working directory.

```basic
cd /harddisk
chdir subdir
```

* Use `/` at the start to go to an absolute location.
* Without `/`, the path is taken relative to your current location.

#### up

Move to the parent directory (one level up).

```basic
up
```

For example, if you are in `/programs/tools`, `up` will take you to `/programs`.

#### run

Run a program from the directory set in `PATH$`.

```basic
run myprog
```

This runs the program named `myprog` from the current program search path.

#### task

Run a program from `PATH$` in the background.

```basic
task myprog
```

The program starts and runs independently, allowing you to continue using the shell.

#### version

Display version and copyright information for the shell.

```basic
version
```

#### push

Save the current working directory on a stack, then optionally change directory.

```basic
push /harddisk
push subdir
push
```

* With a path, the current directory is pushed onto the stack and then changed to the given location.
* Without a path, the current directory is swapped with the most recently pushed directory.
* The stack grows as you use `push`, allowing you to return to earlier locations.

#### pop

Restore the most recently saved directory from the stack.

```basic
pop
```

* Changes the current directory to the last one saved with `push`.
* Removes that entry from the stack.
* If the stack is empty, no change occurs.

#### dirs

Shows all directories pushed to the stack with `push` and `pop`

```basic
dirs
```

#### which

Show which program will be executed for any given command using `PATH$` to resolve it.
`PATH$` is walked from left to right.

```basic
which edit
```

#### history

Show all commands stored in the command history, which you may retrieve with the cursor keys.

```basic
history
```

#### sandbox

Launches a command in a sandboxed environment. The sandboxed environment cannot run various unsafe BASIC functions. All children
of the sandboxed process also inherit the same restrictions.

```basic
sandbox dir /programs
```

**While sandboxed, a BASIC program cannot:**

- Write to or delete files or directories
- Load or unload kernel modules
- Write to or read from raw memory (including buffers)
- Perform raw hardware I/O

### Rocketsh Variables

The rocketsh shell allows you to change certain variables that influence how the shell operates. These are:

#### PATH$

The `PATH$` variable is a list of directories that are searched for programs to run them. Each path in PATH$ (separated
by semicolons, ';') is prepended to the command line to try and match a file. If a match is found by prepending this search
path, the program is executed. So for example if your program is located at `/programs/tasks/foo` and you type `tasks/foo`,
with the path containing `/programs`, the program will be found and ran. Note this is slightly different from how paths operate
in other operating systems. The current directory cannot be included in the path as it has no direct representation.

If you want to run a program that is not in the path, you must provide the full path to the program from the file system root.

#### LIB$

This is the path where system libraries may be found. There can be only one active library path. Programs may optionally
use this global variable when loading their libraries, but it is not mandatory to do so, for example if a program wishes to
load its own libraries from some non-standard location.

#### PROMPT$

Changing the `PROMPT$` variable changes the displayed prompt of the shell to whatever you want. By default, `PROMPT$` has
the value `"ROCKETSH"`. Changing `PROMPT$` to an empty string will cause it to display just `>`, like the BBC Micro.

#### CSD$

You should not directly change the `CSD$` variable. This shows the `CSD`, or 'Currently Selected Directory'.