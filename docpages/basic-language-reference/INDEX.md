\page basic-ref BASIC Language Reference

**Creating BASIC programs**

Programs in Retro Rocket BASIC are similar in structure to other BASIC dialects, with some important differences:

* **Line numbers are optional.**

  * If used, each must be greater than the one before it.
  * Gaps in numbering are allowed.
  * If the first character of the file is not a digit (`0–9`), the program is assumed to have **no line numbers**.

* **Sequential execution.**
  Programs execute line by line, top to bottom, unless redirected with control statements (e.g. `GOTO`, `IF`, `PROC`, `FUNCTION`).

* **Statements.**
  Each line must contain at least one valid statement, with any required parameters.

* **Variables.**
  Four types of variables are available (see \ref variables). Variables may be:

  * **Local** to the current program, or
  * **Inherited** by other programs run with `CHAIN` or similar mechanisms.

For real-world examples of programs in the operating system, see the [os/programs](https://github.com/brainboxdotcc/retro-rocket/tree/master/os/programs) directory in the source tree.

---

**Further Reference**

* \subpage basic-beginner **Beginners’ Guide**
* \subpage variables **Variable Types**
* \subpage keywords **BASIC Keywords**
* \subpage builtin-functions **Built-In Functions**
* \subpage libraries **Libraries**
* \subpage tasks **BASIC Tasks**
* \subpage basic-intdev **Interpreter Development Guide**
