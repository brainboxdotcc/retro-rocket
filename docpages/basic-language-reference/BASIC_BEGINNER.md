\page basic-beginner Beginners' Tutorial

[TOC]

## Getting started

When Retro Rocket is running, you’ll see a place where you can type commands.
This is called the **prompt**.

Think of the prompt like a calculator that understands full programs instead of just sums. You type something in, press Enter, and it runs straight away.

You can type BASIC code directly here and see what it does immediately.

**Single line**

```basic
PRINT "Hello"
```

**Multiple lines**

Use `[` and `]`:

```basic
[
PRINT "Hello"
PRINT "World"
]
```

Press Enter after `]` and it will run.

The square brackets are like telling the computer “here’s a whole block of instructions - run all of this together”.

## 1) Printing text

```basic
[
PRINT "Hello, Retro Rocket!"
]
```

* `PRINT` shows text on the screen
* Text goes inside quotes `"like this"`

Think of `PRINT` like speaking out loud. Whatever you put after it is what the computer “says” back to you.

## 2) Numbers and variables

```basic
[
A = 2
B = 3
SUM = A + B
PRINT SUM
]
```

* `A = 2` stores a value in a variable called `A`
* `SUM = A + B` adds them together
* `PRINT SUM` shows the result

A variable is like a labelled box.
You put a value in the box, and later you can open it and use what’s inside.

Here:

* Box `A` contains `2`
* Box `B` contains `3`
* Box `SUM` contains the result of adding them

## 3) Printing words and values together

```basic
[
A = 2
B = 3
PRINT "2 + 3 = "; A + B
]
```

* `;` joins things on one line

The semicolon is like saying “keep talking”.
It lets you mix words and values in one sentence instead of printing them separately.

## 4) Types of values

**Whole numbers**

```basic
COUNT = 10
```

**Decimal numbers**

```basic
VALUE# = 3.14159
```

**Text**

```basic
NAME$ = "Ada"
```

Think of these like different kinds of items you can put in boxes:

* Whole numbers are like counting objects (1, 2, 3…)
* Decimal numbers are like measurements (3.5 metres, 1.2 litres)
* Text is like labels or names

The symbols help you tell them apart:

* `$` means the box holds text
* `#` means the box holds a decimal number
* no symbol means a whole number

## 5) Getting input

```basic
[
PRINT "What is your name?"
INPUT NAME$
PRINT "Hello "; NAME$
]
```

* `INPUT` waits for you to type something

`INPUT` is like asking a question and waiting for an answer.
Whatever you type gets placed into a variable so the program can use it.

## 6) Decisions (IF)

```basic
[
PRINT "Enter a number"
INPUT N

IF N < 10 THEN
  PRINT "Less than ten"
ELSE
  PRINT "Ten or more"
ENDIF
]
```

An `IF` statement is like a fork in the road.

The program looks at a condition:

* if it’s true, it goes one way
* otherwise, it goes the other

## 7) Loops (repeating things)

**Counting**

```basic
[
FOR I = 1 TO 5
  PRINT I
NEXT
]
```

A `FOR` loop is like counting steps out loud:
“1, 2, 3, 4, 5”

The computer repeats the same instruction for each number.

**Countdown**

```basic
[
N = 5
WHILE N > 0
  PRINT N
  N = N - 1
ENDWHILE
PRINT "Lift off"
]
```

A `WHILE` loop is like saying
“keep going while this is still true”.

It checks the condition each time, and stops when it no longer holds.

## 8) Reusable code

**Procedure**

```basic
[
DEF PROCgreet(NAME$)
  PRINT "Hello "; NAME$
ENDPROC

PROCgreet("Ada")
PROCgreet("Grace")
]
```

A procedure is like writing down a set of instructions you can reuse.

Instead of repeating the same lines over and over, you give them a name and call them when needed.

**Function**

```basic
[
DEF FNadd(A, B)
= A + B

PRINT FNadd(2, 3)
]
```

A function is like a small machine:
you put values in, and it gives you a result back.

Here:

* inputs: `2` and `3`
* output: `5`

## 9) Saving a program

Open the editor:

```basic
edit
```

Type:

```basic
PRINT "Hello from a file"
```

Press **Ctrl+S**, save the file as `/ramdisk/hello` then leave the editor by pressing `ESC`.

Run it from the prompt:

```basic
/ramdisk/hello
```

This example saves to the __ramdisk__, and will remain there until you reboot your computer.

## 10) Small project

```basic
[
SECRET = 7

PRINT "Guess a number from 1 to 10"
INPUT GUESS

IF GUESS = SECRET THEN
  PRINT "Correct"
ELSE
  PRINT "Nope"
ENDIF
]
```

This combines:

* variables (the secret number)
* input (your guess)
* a decision (checking if you’re right)

## 11) Stopping a program

Press **ESC** to stop a running program.

Think of this like pulling the plug if something keeps running longer than you expected.

## 12) Next steps

* Combine `INPUT`, `IF`, and loops
* Try small text games
* Change things and see what happens

You’ve got this-keep experimenting!
