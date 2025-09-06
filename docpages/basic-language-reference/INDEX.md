\page basic-ref BASIC Language Reference

*Creating BASIC programs*

Programs in Retro Rocket are simialar in strucuture to a BBC BASIC program. Each line is numbered, and each number must be greater than the line number before it. It is possible to have gaps in the numbering.

It is possible to not specify the numbers for lines in your program, if you omit line numbers BASIC will auto number your lines for you. Any program who's first character is not a digit (0-9) will be assumed to require auto numbering. Libraries must make use of auto numbering to allow for easy relocation to the end of the program which includes them.

The program will execute moving from one line to the next, and each line must have at least one statement, and any parameters required for that statement (see the statements section of this page).

Variables may be declared, of the four types listed in the section on \ref variables. For each of these variables, they may remain local to the current program or be inherited by other programs ran by the current program.

For an example of programs in the operating system see the [/os/programs](https://github.com/brainboxdotcc/retro-rocket/tree/master/os/programs) directory.

* \subpage basic-beginner Beginners' Guide
* \subpage variables Variable Types
* \subpage keywords BASIC Keywords
* \subpage builtin-functions Built-In Functions
* \subpage libraries Libraries
* \subpage tasks BASIC Tasks
* \subpage basic-intdev Interpreter Development Guide