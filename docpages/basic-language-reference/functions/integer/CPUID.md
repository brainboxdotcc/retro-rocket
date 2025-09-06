\page CPUID CPUID Function
```basic
CPUID(integer-expression, integer-expression, integer-expression)
```
Returns the register value from CPUID. The three parameters are, in order: leaf, subleaf and register value from the table below:

| Value | Register |
|-------|----------|
| 0     | EAX      |
| 1     | EBX      |
| 2     | ECX      |
| 3     | EDX      |

Register values outside of this range will throw an error.

For full details of all possible return values from this function see the [cpuid](../cpuid) command.