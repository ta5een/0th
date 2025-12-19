# Vision & Philosophy

## Purpose

0th serves as a learning experience to understand how software interacts with
hardware at a low level, exploring both programming language development and
operating system development.

## Inspirations

### Self-Contained Systems

- **Smalltalk**: Self-contained environment where the system is built in itself
- **Lisp Machines**: Systems where the OS, tools, and applications are all
  written in Lisp
- **Oberon**: Minimal, self-contained operating system designed for clarity and
  bootstrappability

### Forth Systems

- **Collapse OS / Dusk OS**: Forth-based systems designed as "civilisation
  starters" in case of societal collapse
  - Easily bootstrappable from minimal components
  - Highly extensible once booted
  - Self-contained and simple enough to understand completely

### The Bootstrappable Movement

- **bootstrappable.org**: Philosophy of building systems from minimal,
  auditable components
- Avoiding opaque dependency chains
- Creating reproducible builds from source

## The Vision

Build a set of tools that can be:

1. **Initially compiled** on any system with a C99 compiler
2. **Progressively bootstrapped** until reaching a bootable operating system
3. **Self-contained** - the system, tools, and applications all written in 0th

The C99 "setup" functionality should be minimal - just enough to get the basic
0th compiler running. From there, the system builds itself in 0th.

## The Problem with Conventional Toolchains

Building a non-UNIX operating system conventionally requires a sprawling
dependency chain:

- Build systems (e.g. Meson, CMake)
- Language runtimes (Python for Meson)
- Build tools (e.g. Ninja, Make)
- Shell environments (e.g. Bash)

This creates several issues:

- **Dependency hell**: Each tool brings its own dependencies
- **Unnecessary complexity**: Why should a non-UNIX OS require UNIX-like tools?
- **Loss of understanding**: The toolchain becomes harder to understand than
  the OS itself
- **Brittleness**: Updates to any dependency can break the entire build

## The Forth Solution

Forth offers an elegant alternative:

- **Minimal bootstrap**: Can be implemented in a few hundred lines of C
- **Self-hosting**: The language can compile itself once bootstrapped
- **Unified tooling**: Build tools, compiler, and OS all written in the same
  language
- **Complete understanding**: The entire system can be comprehended by one
  person

## The Bootstrapping Path

1. **Stage 0**: Minimal C99 implementation of 0th compiler
   - Just enough to compile basic 0th code
   - Portable to any C99-compliant platform

2. **Stage 1**: Self-hosting compiler written in 0th
   - Can compile 0th code to native code
   - Compiled by the Stage 0 compiler

3. **Stage 2**: System tools written in 0th
   - Assembler, linker, core utilities
   - All running on native 0th

4. **Stage 3**: Bootable OS kernel
   - Written entirely in 0th
   - Includes basic drivers and system services

5. **Stage 4**: Complete environment
   - Applications, libraries, utilities
   - Fully self-contained system

## Target Platforms

The minimal C99 implementation targets bare-metal environments:

- **Primary target**: RISC-V architecture for its approachable design
- Development and testing via QEMU emulation
- Future expansion to x86 and ARM architectures
- Long-term goal of running on real hardware

Once self-hosting, the system should be able to cross-compile to any supported
target architecture.
