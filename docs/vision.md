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

## Motivation

### The Problem with Conventional Toolchains

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

### The Forth Solution

Forth offers an elegant alternative:

- **Minimal bootstrap**: Can be implemented in a few hundred lines of C
- **Self-hosting**: The language can compile itself once bootstrapped
- **Unified tooling**: Build tools, compiler, and OS all written in the same
  language
- **Complete understanding**: The entire system can be comprehended by one
  person

## Bootstrapping

### The Bootstrapping Path

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

### Why a VM for Stage 0?

The Stage 0 implementation is a VM rather than a compiler. This is a deliberate
choice for the following reasons:

- **Self-definition**: A C compiler would implement code generation in C,
  meaning C determines how 0th compiles to machine code. The VM inverts this:
  C only interprets 0th semantics, while the code generator is written in 0th
  itself. Calling conventions, instruction selection, and code generation are
  expressed in 0th, not inherited from C.
- **Minimal C footprint**: VMs are simpler than compilers - no code generation,
  optimization, or linking logic. Less C code means fewer opportunities for
  C-influenced design decisions to shape the system.
- **Separation of concerns**: The C VM contains no platform-specific code. It
  implements abstract 0th semantics. The Stage 1 code generator (written in
  0th) handles all architecture-specific concerns.
- **Testability**: The VM enables testing 0th programs on the host before
  bare-metal exists, including the Stage 1 code generator itself.

Essentially, the VM is temporary scaffolding. Once bootstrapped, the system is
entirely self-defining - its compilation and runtime behavior determined by 0th
code, not by the C implementation that launched it.

### Bootstrap Verification

The bootstrap process requires verification to prove the 0th compiler is
self-consistent:

1. **C VM compiles 0th compiler** → produces RISC-V binary A
2. **Binary A compiles 0th compiler** → produces RISC-V binary B
3. **Verify A == B** → proves the compiler correctly compiles itself

If A and B are identical, the compiler is self-consistent and the C VM can be
discarded. Binary A (or B) becomes the trusted bootstrap artifact.

## Hosted to Bare-Metal Transition

### I/O Abstraction

The 0th compiler uses abstract I/O words without knowing their implementation:

| Word[^1] | C VM backend | Bare-metal backend    |
|----------|--------------|-----------------------|
| `OPEN`   | `fopen()`    | semihosting SYS_OPEN  |
| `READ`   | `fread()`    | semihosting SYS_READ  |
| `WRITE`  | `fwrite()`   | semihosting SYS_WRITE |
| `CLOSE`  | `fclose()`   | semihosting SYS_CLOSE |

[^1]: The names of these words are subject to change.

This separation means:

- The compiler source is portable across environments
- Same compiler logic produces identical output regardless of host
- Only the I/O primitives need platform-specific implementations

### Semihosting

Semihosting bridges the gap between hosted development and bare-metal
execution. It allows bare-metal code running in QEMU to make calls that the
emulator intercepts and forwards to the host OS, providing file I/O and console
access without an operating system.

This enables the bootstrap verification step: binary A is a bare-metal RISC-V
executable, but when run under QEMU with semihosting, it can still read source
files and write output binaries via the host filesystem.

### Path to Real Hardware

After bootstrap verification, the system transitions from semihosted QEMU to
fully standalone hardware through iterative extension:

1. **Add UART driver** — Serial console I/O that works on real hardware
2. **Add storage driver** — SPI flash or SD card for persistent storage
3. **Remove semihosting** — Replace semihosting calls with hardware drivers

Each step uses the previous compiler to build the next. The chain of trust
extends from the original verified bootstrap.

On real hardware, traditional file I/O may not be needed at all:

- Source stored in flash, interpreted directly
- Forth "blocks" (1KB chunks) instead of files
- Interactive development over serial console
- Compile directly to flash

## Target Platforms

The minimal C99 implementation targets bare-metal environments:

- **Primary target**: RISC-V architecture for its approachable design
- Development and testing via QEMU emulation
- Future expansion to x86 and ARM architectures
- Long-term goal of running on real hardware

Once self-hosting, the system should be able to cross-compile to any supported
target architecture.
