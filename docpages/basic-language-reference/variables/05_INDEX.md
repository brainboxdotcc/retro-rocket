\page variables Variable Types

Variables in Retro Rocket BASIC must follow these rules:

1. Variables **must start with an alphabetic character or an underscore**, e.g. `A`–`Z`, `a`–`z` or `_`.
2. Variables **may contain numeric characters**, but not as the first character.
3. After the first character, a variable **may contain any alphanumeric or underscore character** in any position.
4. Variable names have a **maximum length of 60 characters** (excluding array subscripts). This is to encourage readability.
5. Variable names **may be suffixed** with:

   * `$` → string variable
   * `#` → real (floating-point) variable
     These suffixes are not permitted elsewhere in the name.
6. Variable names are **CaSe SeNsItIvE**.
7. Variable names **cannot start with a keyword or built-in function name**.

---

### Regular expression

The following pattern matches valid Retro Rocket BASIC variable names:

```
/[A-Za-z_][A-Za-z_0-9]{1,58}(\$|#|[A-Za-z_0-9])/
```

---

### Types of Variables

* \subpage type-integer
* \subpage type-real
* \subpage type-string
* \subpage type-array
* \subpage type-bi
