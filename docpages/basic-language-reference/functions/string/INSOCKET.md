\page INSOCKET INSOCKET$ Function
```basic
INSOCKET$(integer-expression)
```
Returns the a character from the socket whos file descriptor is equal to the integer-expression. If there is no data to read, then the function returns an empty string, and if the socket has closed, the function throws an error describing the socket error which occurred.

