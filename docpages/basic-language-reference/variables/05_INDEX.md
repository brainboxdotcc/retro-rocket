\page variables Variable Types

Variables in Retro Rocket BASIC must follow the following rules:

1. Variables **must start with an alphabetic character or an underscore**, e.g. `A`-`Z`, `a`-`z` or `_`.
2. Variables **can contain numeric characters**, so long as those numeric characters are not the first character.
3. After the first character, a variable **may contain any alphanumeric or underscore character in any position**
4. Variable names have a **maximum length of 60 characters**. This does not include any array indexes and is to help aid readability.
5. Variable names **may be suffixed with a single `$` to denote a string variable, or a single `#` to denote a real variable** (floating point). These symbols are not permitted anywhere else in the variable name.
6. Variable names are **CaSe SeNsItIvE**.
7. Variable names **cannot start with a keyword or built in function name**

In summary, the following regular expression could be considered to match all valid Retro Rocket BASIC variables:
```
/[A-Za-z_][A-Za-z_0-9]{1,58}(\$|#|[A-Za-z_0-9])/
```

- \subpage type-integer
- \subpage type-real
- \subpage type-string
- \subpage type-array
- \subpage type-bi
