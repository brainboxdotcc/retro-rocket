\page detok detok command

```
detok <input filename> [output filename]
```

Detokenises a BBC BASIC IV program (BBC model B, B+, Master 128) into plain text. By default, this command will output the detokenised source
code to the screen.

The output filename is optional, if it is specified, then the command will also output the detokenised program to the output file.

Note that you will need to review and adjust BBC BASIC programs for them to run on Retro Rocket. Certain BBC BASIC features such as graphics modes,
raw memory access, etc will need updating to the new dialect.

\image html detok.png
