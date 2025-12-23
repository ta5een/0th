# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

0th is a Forth-inspired environment that bootstraps from a small C99 VM to become a self-hosting system running on bare metal. This is a learning project in early development, currently building the foundation VM.

**Key goals:**
- Self-hosting (bootstrap from C VM to 0th compiler)
- Bare-metal focused (targeting RISC-V, testable via QEMU)
- Minimal bootstrap dependencies (only C99 toolchain required)

**Design philosophy:**
- Prioritize code readability and documentation over performance
- Keep the C99 VM minimal and understandable
- Forth-inspired but not strictly adherent to standards

See `docs/overview.md` and `docs/vision.md` for detailed project goals and long-term bootstrapping roadmap.

## Building and Development

**Quick build:**
```sh
cc -o 0th 0th.c
```

**Development build (with debugging and sanitizers):**
```sh
./build.bash
```

The build script uses strict compilation flags:
- `-std=c99 -pedantic-errors` for C99 compliance
- `-Wall -Werror -Wextra` for strict warnings
- `-fsanitize=address` for memory safety checking

**Code formatting:**
```sh
clang-format -i 0th.c
```

Uses Microsoft style with customizations (see `.clang-format`). Key points:
- 80 column limit
- 4-space indentation
- Pointer alignment right (`Type *ptr`)
- Always break after return type

**Linting:**
```sh
clang-tidy 0th.c
```

Checks enabled: `clang-analyzer-*`, `bugprone-*`, `performance-*`, `portability-*`, `readability-*`

## Development Approach

This is a personal learning project where the maintainer uses LLMs for documentation and planning while maintaining hands-on control of implementation. The project is worked on intermittently, so:

- **Documentation is critical** - Serves as reference when returning after gaps
- **Avoid over-engineering** - Keep implementations simple and minimal
- **Prioritize readability** - Code clarity over performance
- **No vibe-coding** - Thoughtful, deliberate implementation decisions

When making changes:
- Read existing code thoroughly before suggesting modifications
- Document design decisions in comments
- Maintain the minimal, understandable nature of the VM
- Follow the existing comment style (detailed block comments for sections)

## Code Structure

**Single-file design:** The entire VM is in `0th.c` - intentionally monolithic for simplicity and portability. Read the source code and inline comments for architecture details.
