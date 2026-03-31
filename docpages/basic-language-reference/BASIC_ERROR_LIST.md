\page error-list BASIC Error Messages

This page lists BASIC error messages and what they mean.

| Error                                                                 | Description                                                                  |
| --------------------------------------------------------------------- | ---------------------------------------------------------------------------- |
| ACS argument out of range (-1..1)                                   | ACS was given a value outside its valid range.                             |
| Array '\%s' already dimensioned                                      | The array has already been created with DIM.                               |
| Array '\%s': Out of memory                                           | There was not enough memory to create or resize the array.                   |
| Array index \%ld out of bounds                                       | An array index was outside the valid range.                                  |
| Array index \%ld out of bounds [0..\%ld]                              | An array index was outside the valid range shown.                            |
| Array subscripts cannot be negative                                 | An array index below zero was used.                                          |
| Array too small for PUSH [0..\%ld]                                   | PUSH needs more room than the array has.                                   |
| ASN argument out of range (-1..1)                                   | ASN was given a value outside its valid range.                             |
| Bad Address at &\%016lx                                              | A memory access used an invalid address.                                     |
| Bad string literal                                                  | A string constant is malformed.                                              |
| Block IF/THEN/ELSE without ENDIF                                    | A block IF was not closed with ENDIF.                                    |
| COLOUR expected                                                     | A colour value or COLOUR token was expected here.                          |
| CONTINUE FOR used outside FOR                                       | CONTINUE FOR was used when not inside a FOR loop.                        |
| CONTINUE must be followed by WHILE, FOR, or REPEAT                  | CONTINUE needs a loop type after it.                                       |
| CONTINUE REPEAT used outside REPEAT                                 | CONTINUE REPEAT was used when not inside a REPEAT loop.                  |
| CONTINUE WHILE used outside WHILE                                   | CONTINUE WHILE was used when not inside a WHILE loop.                    |
| Call stack depth exceeded                                           | Too many nested calls were made.                                             |
| Call stack exhausted when calling constructor PROC for library '\%s' | The library constructor could not be called because the call stack was full. |
| Can't ENDPROC from a FN                                             | ENDPROC was used inside a function.                                        |
| Can't return a value from a PROC                                    | A procedure tried to return a value with =.                                |
| Can't store socket descriptor in STRING                             | A socket handle cannot be assigned to a string variable.                     |
| Cannot compare string with number                                   | A comparison mixed text and numeric values.                                  |
| Cannot mix string and number with '+' or '-'                        | String and numeric values were combined with + or -.                     |
| Cannot store socket descriptor in REAL                              | A socket handle cannot be assigned to a real variable.                       |
| DATAREAD / DATAREADR / DATAREAD$ related Out of DATA          | A DATAREAD function ran past the available DATA.                         |
| DECIBELS / sound-related out-of-memory messages                     | There was not enough memory for the requested sound operation.               |
| Division by zero                                                    | A division or modulus used zero as the divisor.                              |
| Duplicate function name '\%s'                                        | A DEF FN or DEF PROC name was defined more than once.                    |
| EMPTYRAMDISK$ / RAMDISK$ formatting errors                        | The ramdisk could not be created or formatted.                               |
| ENDPROC when not inside PROC                                        | ENDPROC was used outside a procedure call.                                 |
| EOF is a function                                                   | EOF must be used as a function, not as a statement.                        |
| Empty string                                                        | An operation expected a non-empty string.                                    |
| ENVELOPE number out of range                                        | The envelope number is outside the valid range.                              |
| ENVELOPE: No sound driver is loaded                                 | Sound envelopes cannot be used without a sound driver.                       |
| End of function without returning value                             | A function finished without returning a result.                              |
| Error allocating string buffer                                      | Memory could not be allocated for a string buffer.                           |
| Error reading PROC and FN definitons from library file '\%s'         | Function or procedure definitions could not be read from the library.        |
| Error reading from file: \%s                                         | A file read failed.                                                          |
| Error reading library file '\%s'                                     | The library file could not be read.                                          |
| Error retrieving directory items: \%s                                | A directory listing could not be read.                                       |
| Error retrieving size: \%s                                           | The size of a file or directory entry could not be read.                     |
| Error writing to file: \%s                                           | A file write failed.                                                         |
| Escape                                                              | Execution was interrupted by Escape.                                         |
| Expected \%s got \%s                                                  | The parser found a different token than expected.                            |
| Expected CREATE or DESTROY after ENVELOPE                           | ENVELOPE must be followed by CREATE or DESTROY.                        |
| Expected CREATE or DESTROY after STREAM                             | STREAM must be followed by CREATE or DESTROY.                          |
| Expected PLAY, STOP, PAUSE or VOLUME after SOUND                    | SOUND was followed by an invalid subcommand.                               |
| Expected PROC or OFF                                                | ON ERROR expected PROC or OFF.                                         |
| Expected expression                                                 | An expression was expected here.                                             |
| Expected integer DATA                                               | DATAREAD expected an integer item.                                         |
| Expected real DATA                                                  | DATAREADR expected a real item.                                            |
| Expected string DATA                                                | DATAREAD$ expected a string item.                                          |
| FMOD divide by zero                                                 | FMOD was given zero as the divisor.                                        |
| FOR loop is infinite                                                | The FOR loop step will never reach the end value.                          |
| FOR: Out of memory                                                  | There was not enough memory to create the FOR loop state.                  |
| FN\%s: atomic function timed out                                     | A user-defined function ran too long without finishing.                      |
| Failed to decode first GIF frame '\%s'                               | The first frame of a GIF sprite could not be decoded.                        |
| Failed to format ramdisk '\%s'                                       | The ramdisk filesystem could not be created.                                 |
| Failed to initialise GIF stream '\%s'                                | The GIF decoder could not be initialised.                                    |
| Failed to initialise ramdisk of \%lu sectors                         | The ramdisk backing store could not be set up.                               |
| Failed to load timezone '\%s'                                        | The requested timezone data could not be loaded.                             |
| Failed to read keymap file '\%s'                                     | The keymap file could not be read.                                           |
| Failed to swizzle sprite '\%s'                                       | Sprite pixel data could not be converted into internal format.               |
| GOSUB: stack exhausted                                              | The GOSUB stack is full.                                                   |
| Hexadecimal number too short                                        | A hexadecimal constant ended too early.                                      |
| Invalid ENVELOPE number                                             | The envelope number does not refer to an existing envelope.                  |
| Invalid GIF dimensions                                              | The GIF image has invalid or unsupported dimensions.                         |
| Invalid SLEEP duration                                              | SLEEP was given an invalid delay.                                          |
| Invalid STREAM handle                                               | The stream handle does not refer to a valid sound stream.                    |
| Invalid UDP packet length                                           | The UDP packet size is invalid.                                              |
| Invalid UDP port number                                             | The UDP port number is invalid.                                              |
| Invalid address: \%016lx                                             | A pointer or memory address argument was invalid.                            |
| Invalid array size \%ld                                              | The requested array size is invalid.                                         |
| Invalid character                                                   | The input contains a character that is not valid here.                       |
| Invalid directory '\%s'                                              | The supplied directory path is malformed or invalid.                         |
| Invalid integer variable for DATASET '\%s'                           | DATASET expected a valid integer variable name.                            |
| Invalid length                                                      | A length parameter was invalid.                                              |
| Invalid port for LISTEN                                             | The port number given to LISTEN is invalid.                                |
| Invalid radix                                                       | The radix/base is invalid for this operation.                                |
| Invalid radix (not in range between 2 and 36)                       | The radix must be between 2 and 36.                                          |
| Invalid register                                                    | The specified register is not valid.                                         |
| Invalid socket descriptor                                           | The socket handle does not refer to a valid socket.                          |
| Invalid sprite handle                                               | The sprite handle does not refer to a valid sprite.                          |
| Invalid variable name '\%s'                                          | The variable name is not valid.                                              |
| Keymap file '\%s' not found                                          | The requested keymap file does not exist.                                    |
| LOG argument must be > 0                                            | LOG was given zero or a negative value.                                    |
| Library '\%s': Library files cannot contain line numbers             | A library file included numbered BASIC lines, which are not allowed.         |
| Loading libraries from EVAL is not allowed                          | LIBRARY cannot be used inside EVAL.                                      |
| Malformed hexadecimal number                                        | A hexadecimal constant is malformed.                                         |
| Malformed number                                                    | A numeric constant is malformed.                                             |
| Malformed variable name '\%s'                                        | The variable name is syntactically invalid.                                  |
| MATCH: \%s                                                           | A regular expression operation failed with the message shown.                |
| MATCH: execution error                                              | The regular expression engine failed while matching.                         |
| MATCH: invalid regular expression (\%d)                              | The regular expression pattern is invalid.                                   |
| MATCH: out of memory                                                | There was not enough memory for the regular expression operation.            |
| MATCH: too many capture variables (max \%u)                          | Too many capture variables were requested for MATCH.                       |
| Missing line number after line \%lu: \%s                              | A numbered BASIC program line was malformed or missing its next line number. |
| NEXT without FOR                                                    | NEXT was used without a matching FOR.                                    |
| No DATA                                                             | DATASET or DATAREAD was used when no DATA exists.                      |
| No fill character                                                   | A fill operation expected a single fill character.                           |
| No more sprites available                                           | No free sprite slots remain.                                                 |
| No such PROC \%s                                                     | The named procedure does not exist.                                          |
| No such array variable '\%s'                                         | The named array does not exist.                                              |
| No such integer FN                                                  | The named integer function does not exist.                                   |
| No such integer variable '\%s'                                       | The named integer variable does not exist.                                   |
| No such line \%ld                                                    | The requested line number does not exist.                                    |
| No such real FN                                                     | The named real function does not exist.                                      |
| No such real variable '\%s'                                          | The named real variable does not exist.                                      |
| No such string FN                                                   | The named string function does not exist.                                    |
| No such string variable '\%s'                                        | The named string variable does not exist.                                    |
| Not a directory '\%s'                                                | The path exists but is not a directory.                                      |
| Not a directory: '\%s'                                               | The path exists but is not a directory.                                      |
| Not a file: '\%s'                                                    | The path exists but is not a file.                                           |
| Not a library file: '\%s'                                            | The file is not a valid BASIC library file.                                  |
| Not enough memory for SOUND TONE                                    | There was not enough memory to generate a tone.                              |
| Not enough memory for pitch shift                                   | There was not enough memory to adjust pitch.                                 |
| Not enough memory for sprite canvas '\%s'                            | There was not enough memory for the sprite canvas.                           |
| Not enough memory for sprite mask '\%s'                              | There was not enough memory for the sprite mask.                             |
| Not enough memory for sprite pixels '\%s'                            | There was not enough memory for the sprite pixel data.                       |
| Not enough memory to load library file '\%s'                         | There was not enough memory to load the library file.                        |
| Not enough memory to load sprite file '\%s'                          | There was not enough memory to load the sprite file.                         |
| Number too long                                                     | A numeric constant is too long.                                              |
| Number too short                                                    | A numeric constant ended too early.                                          |
| Numeric operator on string                                          | A numeric operator was used on a string value.                               |
| Numeric value in string expression                                  | A numeric value was used where a string expression was required.             |
| OFF without ON ERROR                                                | ON ERROR OFF was used when no handler was active.                          |
| ON ERROR GOTO and ON ERROR GOSUB are deprecated. Use ON ERROR PROC  | Older ON ERROR forms are no longer supported.                              |
| ON ERROR: No such procedure PROC\%s                                  | The procedure named in ON ERROR PROC does not exist.                       |
| ON ERROR: Out of memory                                             | There was not enough memory to install the ON ERROR handler.               |
| OPENIN is a function                                                | OPENIN must be used as a function, not as a statement.                     |
| OPENOUT is a function                                               | OPENOUT must be used as a function, not as a statement.                    |
| OPENUP is a function                                                | OPENUP must be used as a function, not as a statement.                     |
| Out of DATA                                                         | A DATAREAD function ran past the available DATA.                         |
| Out of STREAMs                                                      | No more sound streams are available.                                         |
| Out of memory                                                       | There was not enough memory to complete the operation.                       |
| Out of memory for CA cert bundle                                    | There was not enough memory to load the CA certificate bundle.               |
| Out of memory loading audio file '\%s'                               | There was not enough memory to load the audio file.                          |
| Out of memory parsing function parameters                           | There was not enough memory to parse function parameters.                    |
| Out of memory parsing functions                                     | There was not enough memory to parse function or procedure definitions.      |
| Out of memory storing DATA                                          | There was not enough memory to store DATA items.                           |
| Out of string area allocator space storing '\%s'                     | The string allocator ran out of space.                                       |
| PROC: stack exhausted                                               | The procedure call stack is full.                                            |
| READ is a function                                                  | READ must be used as a function, not as a statement.                       |
| REPEAT stack exhausted                                              | Too many nested REPEAT loops are active.                                   |
| RETURN without GOSUB                                                | RETURN was used without a matching GOSUB.                                |
| SOUND PLAY: Invalid pitch offset                                    | The pitch offset for SOUND PLAY is invalid.                                |
| SOUND PLAY: Invalid sound handle                                    | The sound handle given to SOUND PLAY is invalid.                           |
| SOUND UNLOAD: Invalid sound handle                                  | The sound handle given to SOUND UNLOAD is invalid.                         |
| SOUND: No sound driver is loaded                                    | Sound output is unavailable because no driver is loaded.                     |
| STREAM: No sound driver is loaded                                   | Sound streams are unavailable because no driver is loaded.                   |
| Sprite file too large                                               | The sprite file is too large to load.                                        |
| Sprite too large: \%dx\%d                                             | The sprite dimensions exceed the allowed size.                               |
| String constant '\%s' too long                                       | A string constant exceeds the maximum length.                                |
| String in numeric expression                                        | A string value was used where a numeric expression was required.             |
| Too many FOR                                                        | Too many nested FOR loops are active.                                      |
| Too many parameters for builtin function                            | Too many arguments were given to a built-in function.                        |
| UDP: Out of memory                                                  | There was not enough memory for the UDP operation.                           |
| UNTIL without REPEAT                                                | UNTIL was used without a matching REPEAT.                                |
| Unary +/- on string                                                 | Unary + or - was applied to a string.                                    |
| Unable to create directory '\%s': \%s                                 | The directory could not be created.                                          |
| Unable to delete directory '\%s': \%s                                 | The directory could not be deleted.                                          |
| Unable to delete file '\%s': \%s                                      | The file could not be deleted.                                               |
| Unable to load CA cert bundle from /system/ssl/cacert.pem           | The CA certificate bundle could not be loaded.                               |
| Unable to load audio file '\%s'                                      | The audio file could not be opened or decoded.                               |
| Unable to load cert \%s: \%s                                          | The certificate file could not be loaded.                                    |
| Unable to load cert: \%s                                             | The certificate file could not be opened or parsed.                          |
| Unable to load key \%s: \%s                                           | The key file could not be loaded.                                            |
| Unable to load key: \%s                                              | The key file could not be opened or parsed.                                  |
| Unable to load module '\%s'                                          | The module could not be loaded.                                              |
| Unable to open cert: \%s                                             | The certificate file could not be opened.                                    |
| Unable to open key: \%s                                              | The key file could not be opened.                                            |
| Unable to open sprite file '\%s'                                     | The sprite file could not be opened.                                         |
| Unable to unload module '\%s'                                        | The module could not be unloaded.                                            |
| Unclosed \%s                                                         | A block or construct was not properly closed.                                |
| Unknown \do_itoa error: \%d                                        | Integer-to-string conversion failed with an unexpected internal error.       |
| Unknown keyword                                                     | The parser found a keyword it does not recognise.                            |
| Unterminated "                                                      | A string literal was not closed.                                             |
| Variable '\%s' already exists as non-array type                      | A scalar variable already exists with that name.                             |
| Variable name too long                                              | The variable name exceeds the maximum length.                                |
| Video flipping is not set to manual mode                            | FLIP was used without manual flipping enabled.                             |
| WHILE stack exhausted                                               | Too many nested WHILE loops are active.                                    |
| up_factor: Out of memory!                                           | Internal scaling failed because memory could not be allocated.               |
| up_value_expr: Out of memory!                                       | Expression evaluation failed because memory could not be allocated.          |

### Notes

* Messages containing \%s, \%ld, \%lx, \%d, or similar placeholders include extra runtime details.
* Some low-level socket and regex errors are passed through from the underlying subsystem and may vary.
* A few messages are very similar because they come from related parts of the interpreter.

**See also:**
\ref BASIC_BEGINNER "Beginners' Tutorial" · \ref ON_ERROR "ON ERROR"
