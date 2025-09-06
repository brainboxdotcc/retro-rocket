\page type-array Array Variables

Arrays of variables are represented by a variable (with a valid suffix or no suffix, as above) and suffixed further by an array *subscript*, e.g. an index into the array, zero based, and surrounded by brackets. For example to access array element 50 from an array of strings you would access it in the form `myarray$(50)`. Arrays must be allocated by the `DIM` statement, and can be dynamically extended using `REDIM`.

As a special case, it is possible to initialise all elements in an array to the same value by not specifying the subscript. For example `arrayname=0` will initialised all array elements to zero and `array$="foo"` will set all elements in a string array to "foo".

