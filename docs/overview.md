# 0th Overview

## What is 0th?

0th is a stack-based language, heavily inspired by Forth. It bootstraps from a
C-based compiler to eventually become self-hosting, with the majority of the
system implemented in 0th itself.

## Project Goals

- **Self-hosting**: Bootstrap from a C-based compiler to compile a new compiler
  written in 0th itself, enabling the system to build itself
- **Bare-metal focused**: Target bare-metal environments across different
  architectures, testable via QEMU with the goal of running on real hardware
- **Minimal bootstrap dependencies**: Initial compiler requires only a C99
  toolchain, making it easily portable

## Design Philosophy

- **Forth-inspired**: Draw inspiration from Forth standards without strict
  adherence
- **Learning-focused**: Prioritize code readability and documentation over
  performance
- **Minimal bootstrap**: Keep the initial C99 compiler minimal and
  understandable
- **Portable foundation**: Leverage C99 for portability, isolate
  architecture-specific code

## Project Status

0th is in early-stage development, currently working toward a functional
prototype.

## Building

To build 0th, you need a C99-compatible compiler such as `clang` or `gcc`:

```sh
cc -o 0th 0th.c
```
