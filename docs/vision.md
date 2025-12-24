# Vision

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

Build a bare-metal operating system that can be:

1. **Bootstrapped** from a minimal C99 VM
2. **Self-hosting** - able to compile itself
3. **Self-contained** - the system, tools, and applications all written in 0th

The C99 VM serves as a hosted development environment for running and testing
0th programs. The key insight: one of those 0th programs is a code generator
that produces RISC-V binaries, enabling the system to build itself for
bare-metal targets.

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

1. **Stage 0**: Minimal C99 VM
   - Runs 0th programs on the host (development environment)
   - Implements core language semantics and stack operations

2. **Stage 1**: RISC-V code generator written in 0th
   - Runs in the Stage 0 VM
   - Generates bare-metal RISC-V binaries
   - Produces bootable images with an interactive 0th environment

3. **Stage 2**: Bootable bare-metal 0th system
   - RISC-V image runs in QEMU
   - Interactive environment for defining and executing 0th words
   - Can generate new code and extend itself

4. **Stage 3**: System tools written in 0th
   - Assembler, linker, core utilities
   - Developed interactively in the bare-metal environment

5. **Stage 4**: Complete self-hosted environment
   - Applications, libraries, utilities
   - Fully self-contained system running on bare-metal

## Why a VM for Stage 0?

The Stage 0 implementation is a VM rather than a compiler. This is a deliberate
choice about how the system becomes self-defining.

**Self-definition**: A C-based compiler would implement the code generator in
C, meaning C code determines how 0th compiles to machine code. The VM approach
inverts this: the C code only interprets 0th semantics (stack operations, word
dispatch), while the code generator is written in 0th itself. The system
defines its own compilation - calling conventions, instruction selection, and
code generation logic are all expressed in 0th, not inherited from C
implementation choices.

**Minimal C footprint**: The VM is simpler to implement than a compiler. No
code generation, optimization passes, or linking logic in C. This minimizes the
C code that must be understood and maintained. Less C code means fewer
opportunities for C-influenced design decisions to shape the system.

**Separation of concerns**: The C VM contains zero platform-specific code. It
implements abstract 0th semantics. Instead, the Stage 1 code generator, written
in 0th and running in the VM, would handle all architecture-specific concerns.
This means a clean boundary between the bootstrap host and target system.

**Testability on hosted environment**: The VM allows testing 0th programs on
the host system before bare-metal is available. Programs can be executed,
debugged, and refined in the hosted environment. This includes developing and
testing the Stage 1 code generator itself.

This treats C99 as temporary scaffolding. Once bootstrapped, the system is
entirely self-defining - its compilation and runtime behavior determined by 0th
code, not by the C implementation that launched it.

## Target Platforms

The minimal C99 implementation targets bare-metal environments:

- **Primary target**: RISC-V architecture for its approachable design
- Development and testing via QEMU emulation
- Future expansion to x86 and ARM architectures
- Long-term goal of running on real hardware

Once self-hosting, the system should be able to cross-compile to any supported
target architecture.
