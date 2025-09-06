\page CALL CALL Keyword
```basic
CALL numeric-address
```

Invokes a **native machine routine** at the given memory address.  
This allows BASIC programs to interface directly with the Retro Rocket kernel or user-supplied machine code.


\remark This feature is **currently not implemented**.
\remark In future, it will allow advanced users to extend BASIC with their own routines.

---

##### Planned Behaviour
- `numeric-address` must be a valid 64-bit address aligned to an instruction boundary.  
- The called routine will execute with the same privileges as the BASIC interpreter.  
- No automatic argument passing or stack frame is provided - users are responsible for adhering to the calling convention.
- Returning from the routine will resume BASIC execution at the next statement.  
- Invalid addresses will raise a runtime error (caught by [`ON ERROR`](https://github.com/brainboxdotcc/retro-rocket/wiki/ONERROR) if present).

---

#### Notes
- This statement is intended for **advanced/low-level use only**.  
- Incorrect usage may crash Retro Rocket or corrupt memory.  
- Future documentation will detail calling conventions and safe entry points.  

**See also:** [`CHAIN`](https://github.com/brainboxdotcc/retro-rocket/wiki/CHAIN), [`LIBRARY`](https://github.com/brainboxdotcc/retro-rocket/wiki/LIBRARY)
