\page error-list BASIC Error Messages

This page lists BASIC error messages and what they mean.

| Error                                                                         | Description                                                                  |
| ----------------------------------------------------------------------------- | ---------------------------------------------------------------------------- |
| ACS argument out of range (-1..1)                                             | ACS was given a value outside its valid range.                               |
| Array 'array_name' already dimensioned                                        | The array has already been created with DIM.                                 |
| Array 'array_name': Out of memory                                             | There was not enough memory to create or resize the array.                   |
| Array index index_value out of bounds                                         | An array index was outside the valid range.                                  |
| Array index index_value out of bounds [0..max_index]                          | An array index was outside the valid range shown.                            |
| Array subscripts cannot be negative                                           | An array index below zero was used.                                          |
| Array too small for PUSH [0..max_index]                                       | PUSH needs more room than the array has.                                     |
| ASN argument out of range (-1..1)                                             | ASN was given a value outside its valid range.                               |
| Bad Address at &hex_address                                                   | A memory access used an invalid address.                                     |
| Bad string literal                                                            | A string constant is malformed.                                              |
| Block IF/THEN/ELSE without ENDIF                                              | A block IF was not closed with ENDIF.                                        |
| COLOUR expected                                                               | A colour value or COLOUR token was expected here.                            |
| CONTINUE FOR used outside FOR                                                 | CONTINUE FOR was used when not inside a FOR loop.                            |
| CONTINUE must be followed by WHILE, FOR, or REPEAT                            | CONTINUE needs a loop type after it.                                         |
| CONTINUE REPEAT used outside REPEAT                                           | CONTINUE REPEAT was used when not inside a REPEAT loop.                      |
| CONTINUE WHILE used outside WHILE                                             | CONTINUE WHILE was used when not inside a WHILE loop.                        |
| Call stack depth exceeded                                                     | Too many nested calls were made.                                             |
| Call stack exhausted when calling constructor PROC for library 'library_file' | The library constructor could not be called because the call stack was full. |
| Can't ENDPROC from a FN                                                       | ENDPROC was used inside a function.                                          |
| Can't return a value from a PROC                                              | A procedure tried to return a value with =.                                  |
| Can't store socket descriptor in STRING                                       | A socket handle cannot be assigned to a string variable.                     |
| Cannot compare string with number                                             | A comparison mixed text and numeric values.                                  |
| Cannot mix string and number with '+' or '-'                                  | String and numeric values were combined with + or -.                         |
| Cannot store socket descriptor in REAL                                        | A socket handle cannot be assigned to a real variable.                       |
| DATAREAD / DATAREADR / DATAREAD$ related Out of DATA                          | A DATAREAD function ran past the available DATA.                             |
| DECIBELS / sound-related out-of-memory messages                               | There was not enough memory for the requested sound operation.               |
| Division by zero                                                              | A division or modulus used zero as the divisor.                              |
| Duplicate function name 'function_name'                                       | A DEF FN or DEF PROC name was defined more than once.                        |
| EMPTYRAMDISK$ / RAMDISK$ formatting errors                                    | The ramdisk could not be created or formatted.                               |
| ENDPROC when not inside PROC                                                  | ENDPROC was used outside a procedure call.                                   |
| EOF is a function                                                             | EOF must be used as a function, not as a statement.                          |
| Empty string                                                                  | An operation expected a non-empty string.                                    |
| ENVELOPE number out of range                                                  | The envelope number is outside the valid range.                              |
| ENVELOPE: No sound driver is loaded                                           | Sound envelopes cannot be used without a sound driver.                       |
| End of function without returning value                                       | A function finished without returning a result.                              |
| Error allocating string buffer                                                | Memory could not be allocated for a string buffer.                           |
| Error reading PROC and FN definitions from library file 'library_file'        | Function or procedure definitions could not be read from the library.        |
| Error reading from file: file_error_message                                   | A file read failed.                                                          |
| Error reading library file 'library_file'                                     | The library file could not be read.                                          |
| Error retrieving directory items: error_message                               | A directory listing could not be read.                                       |
| Error retrieving size: error_message                                          | The size of a file or directory entry could not be read.                     |
| Error writing to file: error_message                                          | A file write failed.                                                         |
| Escape                                                                        | Execution was interrupted by Escape.                                         |
| Expected expected_token got actual_token                                      | The parser found a different token than expected.                            |
| Expected CREATE or DESTROY after ENVELOPE                                     | ENVELOPE must be followed by CREATE or DESTROY.                              |
| Expected CREATE or DESTROY after STREAM                                       | STREAM must be followed by CREATE or DESTROY.                                |
| Expected PLAY, STOP, PAUSE or VOLUME after SOUND                              | SOUND was followed by an invalid subcommand.                                 |
| Expected PROC or OFF                                                          | ON ERROR expected PROC or OFF.                                               |
| Expected expression                                                           | An expression was expected here.                                             |
| Expected integer DATA                                                         | DATAREAD expected an integer item.                                           |
| Expected real DATA                                                            | DATAREADR expected a real item.                                              |
| Expected string DATA                                                          | DATAREAD$ expected a string item.                                            |
| FMOD divide by zero                                                           | FMOD was given zero as the divisor.                                          |
| FOR loop is infinite                                                          | The FOR loop step will never reach the end value.                            |
| FOR: Out of memory                                                            | There was not enough memory to create the FOR loop state.                    |
| FNfunction_name: atomic function timed out                                    | A user-defined function ran too long without finishing.                      |
| Failed to decode first GIF frame 'sprite_file'                                | The first frame of a GIF sprite could not be decoded.                        |
| Failed to format ramdisk 'disk_name'                                          | The ramdisk filesystem could not be created.                                 |
| Failed to initialise GIF stream 'sprite_file'                                 | The GIF decoder could not be initialised.                                    |
| Failed to initialise ramdisk of sector_count sectors                          | The ramdisk backing store could not be set up.                               |
| Failed to load timezone 'timezone_name'                                       | The requested timezone data could not be loaded.                             |
| Failed to read keymap file 'keymap_file'                                      | The keymap file could not be read.                                           |
| Failed to swizzle sprite 'sprite_file'                                        | Sprite pixel data could not be converted into internal format.               |
| GOSUB: stack exhausted                                                        | The GOSUB stack is full.                                                     |
| Hexadecimal number too short                                                  | A hexadecimal constant ended too early.                                      |
| Invalid ENVELOPE number                                                       | The envelope number does not refer to an existing envelope.                  |
| Invalid GIF dimensions                                                        | The GIF image has invalid or unsupported dimensions.                         |
| Invalid SLEEP duration                                                        | SLEEP was given an invalid delay.                                            |
| Invalid STREAM handle                                                         | The stream handle does not refer to a valid sound stream.                    |
| Invalid UDP packet length                                                     | The UDP packet size is invalid.                                              |
| Invalid UDP port number                                                       | The UDP port number is invalid.                                              |
| Invalid address: hex_address                                                  | A pointer or memory address argument was invalid.                            |
| Invalid array size size_value                                                 | The requested array size is invalid.                                         |
| Invalid character                                                             | The input contains a character that is not valid here.                       |
| Invalid directory 'directory_path'                                            | The supplied directory path is malformed or invalid.                         |
| Invalid integer variable for DATASET 'variable_name'                          | DATASET expected a valid integer variable name.                              |
| Invalid length                                                                | A length parameter was invalid.                                              |
| Invalid port for LISTEN                                                       | The port number given to LISTEN is invalid.                                  |
| Invalid radix                                                                 | The radix/base is invalid for this operation.                                |
| Invalid radix (not in range between 2 and 36)                                 | The radix must be between 2 and 36.                                          |
| Invalid register                                                              | The specified register is not valid.                                         |
| Invalid socket descriptor                                                     | The socket handle does not refer to a valid socket.                          |
| Invalid sprite handle                                                         | The sprite handle does not refer to a valid sprite.                          |
| Invalid variable name 'variable_name'                                         | The variable name is not valid.                                              |
| Keymap file 'keymap_file' not found                                           | The requested keymap file does not exist.                                    |
| LOG argument must be > 0                                                      | LOG was given zero or a negative value.                                      |
| Library 'library_file': Library files cannot contain line numbers             | A library file included numbered BASIC lines, which are not allowed.         |
| Loading libraries from EVAL is not allowed                                    | LIBRARY cannot be used inside EVAL.                                          |
| Malformed hexadecimal number                                                  | A hexadecimal constant is malformed.                                         |
| Malformed number                                                              | A numeric constant is malformed.                                             |
| Malformed variable name 'variable_name'                                       | The variable name is syntactically invalid.                                  |
| MATCH: error_message                                                          | A regular expression operation failed with the message shown.                |
| MATCH: execution error                                                        | The regular expression engine failed while matching.                         |
| MATCH: invalid regular expression (error_code)                                | The regular expression pattern is invalid.                                   |
| MATCH: out of memory                                                          | There was not enough memory for the regular expression operation.            |
| MATCH: too many capture variables (max max_count)                             | Too many capture variables were requested for MATCH.                         |
| Missing line number after line line_number: line_text                         | A numbered BASIC program line was malformed or missing its next line number. |
| NEXT without FOR                                                              | NEXT was used without a matching FOR.                                        |
| No DATA                                                                       | DATASET or DATAREAD was used when no DATA exists.                            |
| No fill character                                                             | A fill operation expected a single fill character.                           |
| No more sprites available                                                     | No free sprite slots remain.                                                 |
| No such PROC procedure_name                                                   | The named procedure does not exist.                                          |
| No such array variable 'array_name'                                           | The named array does not exist.                                              |
| No such integer FN                                                            | The named integer function does not exist.                                   |
| No such integer variable 'variable_name'                                      | The named integer variable does not exist.                                   |
| No such line line_number                                                      | The requested line number does not exist.                                    |
| No such real FN                                                               | The named real function does not exist.                                      |
| No such real variable 'variable_name'                                         | The named real variable does not exist.                                      |
| No such string FN                                                             | The named string function does not exist.                                    |
| No such string variable 'variable_name'                                       | The named string variable does not exist.                                    |
| Not a directory 'path'                                                        | The path exists but is not a directory.                                      |
| Not a directory: 'path'                                                       | The path exists but is not a directory.                                      |
| Not a file: 'path'                                                            | The path exists but is not a file.                                           |
| Not a library file: 'file_path'                                               | The file is not a valid BASIC library file.                                  |
| Not enough memory for SOUND TONE                                              | There was not enough memory to generate a tone.                              |
| Not enough memory for pitch shift                                             | There was not enough memory to adjust pitch.                                 |
| Not enough memory for sprite canvas 'sprite_name'                             | There was not enough memory for the sprite canvas.                           |
| Not enough memory for sprite mask 'sprite_name'                               | There was not enough memory for the sprite mask.                             |
| Not enough memory for sprite pixels 'sprite_name'                             | There was not enough memory for the sprite pixel data.                       |
| Not enough memory to load library file 'library_file'                         | There was not enough memory to load the library file.                        |
| Not enough memory to load sprite file 'sprite_file'                           | There was not enough memory to load the sprite file.                         |
| Number too long                                                               | A numeric constant is too long.                                              |
| Number too short                                                              | A numeric constant ended too early.                                          |
| Numeric operator on string                                                    | A numeric operator was used on a string value.                               |
| Numeric value in string expression                                            | A numeric value was used where a string expression was required.             |
| OFF without ON ERROR                                                          | ON ERROR OFF was used when no handler was active.                            |
| ON ERROR GOTO and ON ERROR GOSUB are deprecated. Use ON ERROR PROC            | Older ON ERROR forms are no longer supported.                                |
| ON ERROR: No such procedure PROCprocedure_name                                | The procedure named in ON ERROR PROC does not exist.                         |
| ON ERROR: Out of memory                                                       | There was not enough memory to install the ON ERROR handler.                 |
| OPENIN is a function                                                          | OPENIN must be used as a function, not as a statement.                       |
| OPENOUT is a function                                                         | OPENOUT must be used as a function, not as a statement.                      |
| OPENUP is a function                                                          | OPENUP must be used as a function, not as a statement.                       |
| Out of DATA                                                                   | A DATAREAD function ran past the available DATA.                             |
| Out of STREAMs                                                                | No more sound streams are available.                                         |
| Out of memory                                                                 | There was not enough memory to complete the operation.                       |
| Out of memory for CA cert bundle                                              | There was not enough memory to load the CA certificate bundle.               |
| Out of memory loading audio file 'audio_file'                                 | There was not enough memory to load the audio file.                          |
| Out of memory parsing function parameters                                     | There was not enough memory to parse function parameters.                    |
| Out of memory parsing functions                                               | There was not enough memory to parse function or procedure definitions.      |
| Out of memory storing DATA                                                    | There was not enough memory to store DATA items.                             |
| Out of string area allocator space storing 'string_value'                     | The string allocator ran out of space.                                       |
| PROC: stack exhausted                                                         | The procedure call stack is full.                                            |
| READ is a function                                                            | READ must be used as a function, not as a statement.                         |
| REPEAT stack exhausted                                                        | Too many nested REPEAT loops are active.                                     |
| RETURN without GOSUB                                                          | RETURN was used without a matching GOSUB.                                    |
| SOUND PLAY: Invalid pitch offset                                              | The pitch offset for SOUND PLAY is invalid.                                  |
| SOUND PLAY: Invalid sound handle                                              | The sound handle given to SOUND PLAY is invalid.                             |
| SOUND UNLOAD: Invalid sound handle                                            | The sound handle given to SOUND UNLOAD is invalid.                           |
| SOUND: No sound driver is loaded                                              | Sound output is unavailable because no driver is loaded.                     |
| STREAM: No sound driver is loaded                                             | Sound streams are unavailable because no driver is loaded.                   |
| Sprite file too large                                                         | The sprite file is too large to load.                                        |
| Sprite too large: widthxheight                                                | The sprite dimensions exceed the allowed size.                               |
| String constant 'string_value' too long                                       | A string constant exceeds the maximum length.                                |
| String in numeric expression                                                  | A string value was used where a numeric expression was required.             |
| Too many FOR                                                                  | Too many nested FOR loops are active.                                        |
| Too many parameters for builtin function                                      | Too many arguments were given to a built-in function.                        |
| UDP: Out of memory                                                            | There was not enough memory for the UDP operation.                           |
| UNTIL without REPEAT                                                          | UNTIL was used without a matching REPEAT.                                    |
| Unary +/- on string                                                           | Unary + or - was applied to a string.                                        |
| Unable to create directory 'directory_path': error_message                    | The directory could not be created.                                          |
| Unable to delete directory 'directory_path': error_message                    | The directory could not be deleted.                                          |
| Unable to delete file 'file_path': error_message                              | The file could not be deleted.                                               |
| Unable to load CA cert bundle from /system/ssl/cacert.pem                     | The CA certificate bundle could not be loaded.                               |
| Unable to load audio file 'audio_file'                                        | The audio file could not be opened or decoded.                               |
| Unable to load cert certificate_file: error_message                           | The certificate file could not be loaded.                                    |
| Unable to load cert: error_message                                            | The certificate file could not be opened or parsed.                          |
| Unable to load key key_file: error_message                                    | The key file could not be loaded.                                            |
| Unable to load key: error_message                                             | The key file could not be opened or parsed.                                  |
| Unable to load module 'module_name'                                           | The module could not be loaded.                                              |
| Unable to open cert: error_message                                            | The certificate file could not be opened.                                    |
| Unable to open key: error_message                                             | The key file could not be opened.                                            |
| Unable to open sprite file 'sprite_file'                                      | The sprite file could not be opened.                                         |
| Unable to unload module 'module_name'                                         | The module could not be unloaded.                                            |
| Unclosed construct                                                            | A block or construct was not properly closed.                                |
| Unknown do_itoa error: error_code                                             | Integer-to-string conversion failed with an unexpected internal error.       |
| Unknown keyword                                                               | The parser found a keyword it does not recognise.                            |
| Unterminated "                                                                | A string literal was not closed.                                             |
| Variable 'variable_name' already exists as non-array type                     | A scalar variable already exists with that name.                             |
| Variable name too long                                                        | The variable name exceeds the maximum length.                                |
| Video flipping is not set to manual mode                                      | FLIP was used without manual flipping enabled.                               |
| WHILE stack exhausted                                                         | Too many nested WHILE loops are active.                                      |
| up_factor: Out of memory!                                                     | Internal scaling failed because memory could not be allocated.               |
| up_value_expr: Out of memory!                                                 | Expression evaluation failed because memory could not be allocated.          |

**See also:**
\ref BASIC_BEGINNER "Beginners' Tutorial" · \ref ON-ERROR "ON ERROR"
