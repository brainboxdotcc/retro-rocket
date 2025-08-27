# Retro Rocket BASIC — Beginner Tutorial

Welcome! This hands‑on guide is for absolute beginners. You’ll learn the basics by writing and running tiny programs inside Retro Rocket. No prior programming experience needed.

> **How this works**\
> • Open the built‑in editor with `edit <name>`\
> • Save with **Ctrl+S**.\
> • Run from the shell by typing its name you saved it as.\
> • Press **CTRL+ESC** while a program is running to stop it.

We’ll start simple and build up.

---

## 1) Your first program: printing text

Open the editor:

```
edit
```

Type this and save (Ctrl+S) with the name: `hello`

```
PRINT "Hello, Retro Rocket!"
```

Leave the editor, then run it:

```
hello
```

You should see the message appear. That’s your first program!

**Notes**

- `PRINT` writes text to the screen.
- Quotes surround literal text (called a *string*).

---

## 2) Variables and simple maths

Open a new file:

```
edit
```

Type and save (Ctrl+S) as `mathsdemo`:

```
A = 2
B = 3
SUM = A + B
PRINT "2 + 3 = "; SUM
```

Run it:

```
mathsdemo
```

**What’s happening**

- You create *variables* with `=`. Names like `A`, `B`, `SUM` hold numbers.
- `PRINT ...; ...` prints items on one line (the semicolon avoids an automatic newline between parts).

**Three value types you’ll meet**

- **Integers** (whole numbers) — default: `AGE = 12`
- **Reals** (decimals) — suffix `#`: `PI# = 3.14159`
- **Strings** (text) — suffix `$`: `NAME$ = "Ada"`

Use the right suffix when you want a real or string. Integers have no suffix.

---

## 3) Getting input and using strings

Open:

```
edit
```

Type and save (Ctrl+S) as `hello_you`:

```
PRINT "What is your name?"
INPUT NAME$
PRINT "Nice to meet you, "; NAME$
```

Run:

```
hello_you
```

**What’s happening**

- `INPUT` waits for you to type a line and press Enter, then stores it in a variable.
- String variables end with `$`.

---

## 4) Making decisions (IF / ELSE)

Open:

```
edit
```

Type and save (Ctrl+S) as `agegate`:

```
PRINT "How old are you?"
INPUT AGE
IF AGE < 13 THEN
  PRINT "Hi there!"
ELSE
  PRINT "Welcome."
ENDIF
```

Run it and try different ages.\
`IF ... THEN` chooses a path; `ELSE` is optional; `ENDIF` closes the block.

---

## 5) Repeating work (FOR and WHILE)

**A counting loop**

```
FOR I = 1 TO 5
  PRINT "Number "; I
NEXT
```

**A condition‑driven loop**

```
N = 5
WHILE N > 0
  PRINT N
  N = N - 1
ENDWHILE
PRINT "Lift‑off!"
```

---

## 6) Breaking code into procedures and functions

Procedures are reusable blocks that don’t return a value. Functions return a value.

**Procedure example**

```
DEF PROC Greet(NAME$)
  PRINT "Hello, "; NAME$
ENDPROC

PROC Greet("Ada")
PROC Greet("Grace")
```

**Function example**

```
DEF FN Add(A, B)
 = A + B

PRINT "2 + 3 = "; FN Add(2, 3)
```

**Notes**

- Call a procedure with `PROC Name(...)`.
- Call a function with `FN Name(...)`. The line starting with `=` returns the value.
- Prefer `PROC`/`FN` over older styles like `GOTO`/`GOSUB`.

---

## 7) A tiny project: Guess the number

```
edit
```

Type and save (Ctrl+S) as `guess`:

```
PRINT "I am thinking of a number between 1 and 10."
SECRET = 7
PRINT "Your guess?"
INPUT GUESS
IF GUESS = SECRET THEN
  PRINT "Correct!"
ELSE
  PRINT "Not this time."
ENDIF
```

Run with `guess`. Improve it by looping until correct, counting attempts, or giving hints (`IF GUESS < SECRET THEN ...`).

---

## 8) Tips and troubleshooting

- **Stopping a program:** press **ESC**. (Advanced: you can catch this with `ON ERROR PROC ...` and check `ERR$`.)
- **Saving:** remember **Ctrl+S** in the editor.
- **Printing tricks:**
  - Use a comma to tab‑separate: `PRINT A, B, C`
  - Use semicolon to join on one line: `PRINT "X="; X; " Y="; Y`

---

## 9) Where to go next

- Try combining `INPUT`, `IF`, and loops to make simple text games.
- Learn about arrays and sprites when you’re comfortable with the basics.
- Browse the separate “BASIC.md” internals doc if you’re curious how the interpreter works under the hood.

You’ve got this—keep experimenting!

