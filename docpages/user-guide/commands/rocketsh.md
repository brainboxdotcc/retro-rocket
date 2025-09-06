\page rocketsh rocketsh command (shell)

The interactive shell used to execute commands and run programs. Launched by `init`.

![image](https://user-images.githubusercontent.com/1556794/232544878-7dc6d1a7-358c-400e-9aaa-3346aaa7bbb5.png)

#### Command line entry

The `rocketsh` shell supports command line editing. You may use the cursor `LEFT` and `RIGHT` keys plus `HOME` and `END` to navigate in the current line and edit it, inserting into the line if neccessary.

Alternately, you can use the cursor `UP` and `DOWN` keys to move through the history of recently entered commands. If you edit one of these commands or send it, it will be inserted into the history again as the latest command.

The history buffer will grow as needed to accomodate as many history items as you enter.

Pressing `ESC` will abort the current line entry, clearing its contents and advancing down to the line below.

These interactive line editing facilities are also available to any other BASIC programs via the `ansi` library.

#### Rocket Shell Commands

The `rocketsh` shell supports two forms of input; firstly any command you type is searched for as a program under the `/programs` directory. If it can be found it is executed. The path under which `rocketsh` searches for programs to run can be changed, as shown below.

If no matching program can be found, the line is passed to [EVAL](https://github.com/brainboxdotcc/retro-rocket/wiki/EVAL) instead and any changes to the program state will be inherited by `rocketsh`, including documented variables below.

#### Stopping running programs

Any running program may be stopped by pressing CTRL+ESC at any time. This will raise an error in the program and if it does not trap the error, it will be ended. Errors of this nature in `rocketsh` are gracefully handled, and will end entry of the current command line.

#### Shell Variables

The `rocketsh` shell has several documented variables, which can be set via the command prompt. These are:

##### PROMPT$

Change `PROMPT$` to change the shell prompt from the default of `ROCKETSH` to the specified value. ANSI escape sequences are supported. You can use `CHR$(27)` to insert the escape chracter:

```basic
PROMPT$ = CHR$(27) + "[31mRED PROMPT" + CHR$(27) + "[37m"
```

and this will set the shell prompt to dark red:

![image](https://user-images.githubusercontent.com/1556794/235948821-9a866d75-a5dd-4ba2-b94e-f57f1ba308dd.png)

##### PATH$

The directory to search in for programs ran at the command prompt. It defaults to `/programs`.

```BASIC
PATH$ = "/harddisk/progs"
```

##### GLOBAL LIB$

The `GLOBAL` variable LIB$ is the path that programs will use to load libraries from within BASIC. It defaults to `/programs/libraries` and is set from within `/programs/init` and passed down to `rocketsh`.

You must remember the `GLOBAL` prefix if you wish for this value to be inherited by BASIC programs.

```BASIC
GLOBAL LIB$ = "/harddisk/progs/lib"
```

##### EDhist$(x)

Returns a value from the edit history. Each time a line is typed into `rocketsh` it is stored into the edit history, with the most recent line stored at array index 0.

#### EDhistPtr

Returns the last array index in the edit history array which contains valid data.
