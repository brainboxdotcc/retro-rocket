\page LOG LOG Function

```basic
LOG(real-expression)
```

Returns the **natural logarithm** (base *e*, approximately `2.71828`) of the given expression.

The input must be strictly greater than `0`.

---

### Examples

```basic
PRINT LOG(1)
```

Produces `0`.

```basic
PRINT LOG(EXP(3))
```

Produces approximately `3`.

```basic
REM Convert exponential growth back to linear scale
value# = 1000
PRINT "ln("; value#; ") = "; LOG(value#)
```

---

### Notes

* Domain: input must be `> 0`.
* Range: result may be any real number.
* Inverse of \ref EXP "EXP": `LOG(EXP(x)) = x`.
* To compute logarithms in other bases:

  ```basic
  LOG10(x#) = LOG(x#) / LOG(10)
  ```

---

**See also:**
\ref EXP "EXP" · \ref POW "POW" · \ref SQR "SQR"
