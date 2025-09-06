\page TOKENIZE TOKENIZE$ Function
```basic
TOKENIZE$(string-variable, string-expression)
```
Searches for the first occurance of the string-expression value within the value of the string-variable given, splitting the value at that point and returning the portion of the string before the split, changing the string to become the content that came after the split. For example:

![image](https://user-images.githubusercontent.com/1556794/235389891-e8973648-eb27-40b5-b7e2-35c02baaf311.png)

The string-expression is not limited to one character, if a multiple character string value is specified, this will be the whole separator.