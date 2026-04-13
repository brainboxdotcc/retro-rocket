\page tree tree command

```
tree [directory]
```

Displays a directory and all its subdirectories in a tree-like view.
This makes it easy to see the structure of the file system at a glance.

### Usage
```
tree
tree /programs
tree /system/webserver
```

\image html tree.png

### Notes
- If no directory is specified, `tree` starts at the current working directory.
- If the current directory is empty, nothing is printed except the summary.
