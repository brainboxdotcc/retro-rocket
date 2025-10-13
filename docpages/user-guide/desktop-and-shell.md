\page shell The Shell

When Retro Rocket finishes booting you arrive at the **command shell** (called `rocketsh`).
This is a text-based interface where you type commands at a prompt:

\image html shell.png

The shell is the starting point for everything you do in Retro Rocket.
From here you can explore files, run BASIC programs, or start writing your own.

### Using the shell
- Type a command at the prompt and press **Enter**.
- Commands that start external programs are generally written in **lowercase**, so it is obvious you are running an external program.
- External programs are looked for in the `/programs` directory.
- BASIC keywords are always in **uppercase**.

### Editing commands
- Use the **arrow keys** to move through the line and edit it.
- Use **UP/DOWN** arrows to move through recently entered commands.
- Pressing **ESC** clears the current line.

These editing features are shared with BASIC programs that use the `ansi` library.

### More about rocketsh
The shell has its own variables (`PROMPT$`, `PATH$`, and others) that let you customise it.
For details, see the @ref rocketsh "rocketsh command reference".
