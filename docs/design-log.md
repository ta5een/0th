# Design Log

## 2026-05-22

### Bootstrapping Approach Discussion

- Returned to the project after a few months. The main open question was the bootstrapping approach — two options on the table:
  - **Code-generator approach:** The C99 VM interprets 0th. A code generator written in 0th produces bare-metal binaries for each target. C VM is temporary scaffolding, discarded once bootstrapped. 0th truly controls its own destiny.
  - **Hand-written kernels approach:** A kernel per architecture, each implementing the same primitive interface. The C99 VM is a permanent hosted kernel. The same `bootstrap.0th` runs on all kernels unchanged. "Self-hosting" means self-extending (like a Lisp Machine), not self-compiling.
- Key clarification: a kernel *cannot* be written in 0th. The kernel *is* the primitive implementations, and primitives are defined as things that can't be expressed in 0th. The only way out of that circle is to have a running 0th system emit machine code for a new kernel — which is exactly the code-generator approach. The two approaches are the two sides of the same coin.
- The approaches also differ in assembler dependencies:
  - Assembly kernels require an assembler (GNU as, LLVM, etc.) as a bootstrap dependency
  - C99 kernels require a cross-compiler — same dependency class as what's already needed
  - Or: a C99 host program that emits raw kernel bytes directly, keeping bootstrap dependencies at strictly "host C99 compiler only" — the approach taken by the bootstrappable builds movement
- The bootstrapping approach is still undecided. Both approaches share the same immediate foundation: settling the primitive interface, finishing `bootstrap.0th`, and building enough of the 0th system to feel what it's like to program in it. The decision will likely become clearer once that foundation exists.
- The docs (overview.md, vision.md) still describe the code-generator approach and need updating once the decision is made.

### Ken Thompson - Reflections on Trusting Trust (1984)

- A compiler can contain a self-replicating backdoor that never appears in source code. Source code auditability alone is insufficient — you have to trust the entire chain that produced your tools.
- The bootstrappable builds response: start from the smallest possible auditable seed (a handful of hex bytes a human could verify by hand) and bootstrap up from there.
- Relevant to 0th: the C99 VM is the trusted seed. The primitive set is small enough that a careful reader could audit it entirely — closer to the bootstrappable ideal than most systems.

### Primitive Set Gaps

- The current primitive set (derived from milliforth/plankforth) is missing:
  - Output: `k` reads a character but there's no emit primitive
  - Bitwise ops: AND, OR, NOT, XOR — can't be derived portably from arithmetic
  - `{` and `}` (begin/end definition) — used throughout `bootstrap.0th` but not documented in the primitive list
  - Return stack access — needed for certain control flow and local variables
  - Less-than comparison — `=` exists but no `<` or `>`
  - Multiply and divide — impractical to build from repeated addition
