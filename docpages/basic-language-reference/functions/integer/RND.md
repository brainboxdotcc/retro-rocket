\page RND RND Function
```basic
RND(integer-expression, integer-expression)
```
Returns a pseudo-random number between the two integer expressions (inclusive). The pseudo-random number generator uses a [Mersenne Twister mt19937](https://en.wikipedia.org/wiki/Mersenne_Twister) algorithm to produce random results, with entropy seeded from the operating system kernel via various inputs which are not directly under user control.

![image](https://user-images.githubusercontent.com/1556794/235545663-e3b17cdf-c47e-4d65-bfa1-84b3902c47fe.png)

**NOTE: This differs from BBC BASIC which used a simple 33-bit LFSR (Linear Feedback Shift Register) as opposed to Mersenne Twister.**

This function should not be relied upon for cryptographic randomness as its seed value cannot be guaranteed secure.