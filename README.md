# CCC - COIL C Compiler

CCC is a simple C compiler that generates COIL (Computer Oriented Intermediate Language) binary format directly, skipping the CASM assembly stage.

## Features

- Compiles a subset of C directly to COIL binary format (.coil files)
- Utilizes the libcoil-dev library for COIL binary generation
- Simple and efficient compilation pipeline
- Supports basic C constructs including:
  - Variables and basic types (int, float, char, pointers)
  - Control flow (if, while, for)
  - Functions
  - Basic arithmetic and logic operations
  - Arrays and pointers

## Building

CCC uses the Meson build system. To build:

```bash
meson setup builddir
cd builddir
meson compile
```

## Dependencies

- C++17 compatible compiler
- Meson build system
- libcoil-dev library

## Usage

```bash
ccc [options] input.c -o output.coil

Options:
  -o <file>     Specify output file (default: a.coil)
  -O<level>     Optimization level (0-3)
  -I <dir>      Add include directory
  -D <name>[=value] Define macro
  -v            Verbose output
  -h, --help    Display help
```

## Example

```bash
# Compile a C file to COIL
ccc example.c -o example.coil

# Process with COIL processor
coilp example.coil -o example.coilo

# Link with COIL-specific linker
cld example.coilo -o example
```

## License

This project is released under the Unlicense, same as libcoil-dev.