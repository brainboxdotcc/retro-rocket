\page RAMDISK RAMDISK$ Function
```basic
RAMDISK$(string-expression)
```
Create a ramdisk device with the content of a separate block device who's name is in the string-expression. Returns the name of the new block device.

\page RAMDISK RAMDISK Function
```basic
RAMDISK(integer-expression, integer-expression)
```
Create a ramdisk device of X blocks each block being Y bytes, where X and Y are the two integer-expression values. Note that the block device will be unformatted, and cannot be immediately mounted to a mountpoint. Returns the name of the new block device.

