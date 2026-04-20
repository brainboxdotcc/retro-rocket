\page datetime Date and Time Library

```BASIC
LIBRARY LIB$ + "/datetime"
```

The datetime library provides simple utilities for working with Unix timestamps.

It supports conversion from a Unix timestamp (seconds since 1 January 1970 UTC) into a human-readable date/time string. The implementation is lightweight and does not rely on external systems.

The following publicly documented procedures and functions are available via this library.

---

## Utility functions

### FNpad$(n, width)

Return a zero-padded string representation of integer `n` with minimum width `width`.

Example:

```BASIC
PRINT FNpad$(5, 2)
```

Outputs:

```
05
```

---

### FNisLeap(y)

Return `TRUE` if year `y` is a leap year, otherwise `FALSE`.

Leap year rules follow the Gregorian calendar:

* divisible by 400 → leap year
* divisible by 100 → not a leap year
* divisible by 4 → leap year

---

## Time conversion

### FNunixtime$(ts)

Convert a Unix timestamp `ts` (seconds since 1970-01-01 00:00:00 UTC) into a formatted date/time string.

The returned format is:

```
YYYY-MM-DD HH:MM:SS
```

Example:

```BASIC
PRINT FNunixtime$(0)
```

Outputs:

```
1970-01-01 00:00:00
```

---

## Behaviour

* Time is interpreted as **UTC**
* Leap years are correctly handled
* Month lengths are adjusted dynamically for leap years
* No timezone or locale support is provided

---

## Example

```BASIC
LIBRARY LIB$ + "/datetime"

PROCdatetime

ts = 1713550000
PRINT FNunixtime$(ts)
```

---

## Notes

* The library performs calculations using integer arithmetic only
* It is suitable for formatting timestamps received from external systems (e.g. HTTP headers, APIs)
* Negative timestamps (dates before 1970) are not explicitly handled
* No parsing functions (string → timestamp) are currently provided
