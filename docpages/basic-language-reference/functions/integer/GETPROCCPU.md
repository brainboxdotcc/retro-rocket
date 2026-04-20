\page GETPROCCPU GETPROCCPU Function

```basic
V = GETPROCCPU(pid)
```

Returns the rolling average CPU usage of the process identified by `pid` as an integer percentage from `0` to `100`.

A process that is waiting or suspended reports `0`. A runnable process reports its recent share of CPU time, averaged over a rolling window, so the value may take a short time to rise or fall.

### Notes

`GETPROCCPU` returns a recent average, not an instant reading.

The value is based on the scheduler's view of the process and is intended for status displays such as task managers or monitoring tools.

If `pid` does not refer to a valid process, the function returns `0`.

### Errors

No error is raised for an invalid PID. The function returns `0`.

### Examples

```basic
PRINT "This process is using "; GETPROCCPU(PID); "% CPU"
```

