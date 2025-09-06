\page GETPROCSTATE GETPROCSTATE$ Function
```basic
GETPROCSTATE$(integer-expression)
```
Returns the process state for a running process, the integer expression is an index between 0 and `GETPROCCOUNT - 1`

The process status is one of:

| State     | Description                                                    |
|-----------|----------------------------------------------------------------|
| running   | process is actively executing                                  |
| suspended | process is waiting on another process to end before continuing |
| waiting   | process is waiting on I/O, e.g. disk, console, network         |
| ended     | process no longer exists                                       |
| unknown   | an error state, the status of this process is unknown          |
