# stac
simple dialect of forth implemented in c

## Building
Run `make` to build the default target.  This will generate a system dependent include file, and compile the whole program.
For Debug, run `make debug`.  You can then attach to the process using `sudo gdb -p [proc id of stac]`

## Running
stac can be run by `cat core.stc - | ./stac`
If you have a stac program, you can run it by `cat core.stc [program.stc] | ./stac`

## Internals
### Disk
The disk implementation is composed of a number of integer-sized cells.  It holds the data stack and return stacks at the higher end of the disk, with space for the dictionary at the beginning. It also holds a number of in-disk registers, namely the dictionary pointer `dict`, return stack pointer `rsp`, data stack pointer `tosp`, and dictionary link pointer `link`.

### Stacks
Stack pointers hold the location of the next available cell in the stack, with the data stack growing from the end of the disk towards the return stack, and the return stack growing from a fixed offset towards the data stack.

### Dictionary
The dictionary is a simple linked list structure with every entry containing a link to the previous entry, a name associated with that entry (as a counted string), an immediate flag, and the actual data.  The `dict` pointer holds the location of the next available cell in the dictionary, growing from the beginning of the disk.  The `link` pointer holds the location of the last link in the dictionary.

### Strings
stac uses counted strings rather than null-delimited strings, so in memory it basically contains an int, followed by that many chars.  Because the cell size is the size of an int, this can result in a partially filled cell, which we then skip in the dictionary to allow for full usage of the next cell.
