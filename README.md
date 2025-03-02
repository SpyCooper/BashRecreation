# Bash Recreation

A basic recreation of a bash environment.

## Description

This was a program made for my Operating Systems class that recreated a bash environment. The following commands were built into the environment:

- `echo`: print out all the arguments
- `cd`: changes the working directory
- `cd dir`: changes the directory to `dir`
- `$VAR`: environment variables and variable replacement in other commands work. Using `PS1` to change the starting command seems to be a bit off from a normal bash environment
- `|`: pipe redirection for all commands
- `<`: standard in redirection for all commands
- `>`: standard out redirection for all commands
- `>>` standard out append redirection for all commands
- `2>` standard error redirection for all commands
- `2>>` standard error append redirection for all commands
- `'` and `"` work for all commands. If something is in `"`, variables will need to be expanded, while `'` are left alone. This also works with nested versions of each of these

All other commands are ran using `exec`.

## Execution

The `Makefile` will compile the `scanner.l`, `parser.y`, `bash.h`, and `bash.cc` together into an executable `bash`. The `Makefile` can run any of the following commands:

- `make`: compile all the files into the `bash` executable
- `make test`: compiles all the files and runs all tests and prints out if they passed or not
- `make clean`: cleans all the files that are generated by the `make` and `make test` commands

Any individual test can be run by running `./test.#` with te # being which test is to be run (example: `./test.1`)

## Credit

The test files, `Makefile`, and the original outline for the files were made by Dr.Shawn Ostermann.

The logic and most of the `scanner.l`, `parser.y`, and `bash.cc` were made by me.