## UID: 123456789

## Pipe Up

Pipe Up is implementing the command line pipe operator by redirecting file descriptors

## Building

to build the program, run:

make

if on macOS: Comment out line in Makefile with LDFLAGS

## Running

Show an example run of your program, using at least two additional arguments, and what to expect
After running: 

ls | cat | wc 

the stdout was:

       7       7      63

To try to replicate this, I ran:

./pipe ls cat wc 

and the stdout was: 

       7       7      63


## Cleaning up

In order to clean up binary files, run:

make clean
