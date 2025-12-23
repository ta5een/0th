# 0th

0th is a Forth-inspired environment that bootstraps from a small C99 VM to
become a self-hosting system running on bare metal.

**Status**: Early development, currently building the foundation VM.

## Building

To build 0th, you need a C99-compatible compiler such as `clang` or `gcc`:

```sh
cc -o 0th 0th.c
```

## Further Reading

- [Overview](docs/overview.md) - Project goals and design philosophy
- [Vision](docs/vision.md) - Bootstrapping path and long-term vision
- [References](docs/references.md) - Forth resources and related projects

## Development

This is a personal learning project exploring low-level systems programming and
language design. I use LLMs (like Claude) to help translate ideas into
documentation and organize tasks, while maintaining hands-on control of all
implementation decisions.

Since this is a side project worked on intermittently, documentation serves as
a record for my future self when returning after gaps. It also forces me to
think through design decisions clearly and provides a reference point when
implementing, making the project more approachable for both myself and others.
