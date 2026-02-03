# Bootstrapping Approach: Questions to Consider

This document captures questions to help decide between two bootstrapping
approaches for 0th.

## The Two Approaches

**Previous (0th-written code generator):**

- C99 VM interprets 0th code
- Write a RISC-V code generator in 0th
- That code generator produces bare-metal binaries
- Bootstrap verification: binary A compiles the compiler, producing binary B;
  verify A == B
- C99 VM is temporary scaffolding, discarded after bootstrapping

**Proposed (hand-written kernels):**

- Hand-written kernel per platform (C99 VM in C, RISC-V in assembly, etc.)
- Each kernel implements the same primitive interface (`: ; COMPILE,` etc.)
- bootstrap.0th is portable across all kernels
- No 0th-written code generator
- C99 VM is a permanent peer alongside bare-metal targets

---

# Personal / Why

## On Learning Goals

**What do you want to understand deeply by the end of this project - code
generation, or how different architectures execute code?**

>

**Would writing a RISC-V code generator in 0th teach you things that writing a
RISC-V kernel in assembly wouldn't (or vice versa)?**

>

**Is the journey (building the system) or the destination (having a self-hosted
system) more important?**

>

---

## On Momentum and Motivation

**This is a project you work on intermittently. Which approach gives you more
frequent "it works!" moments to maintain momentum?**

>

**What's the minimum viable 0th that would satisfy you? At what point would you
feel "done enough"?**

- Reaching a point where I can self-host 0th and make changes to 0th right in
  the 0th system itself would satisfy me the most.
  - I should be able to edit 0th source, apply changes in some way, and observe
    these changes in the system once applied/committed

---

## On Audience

**Is 0th purely for your own learning, or do you hope others could learn from
it too (like jonesforth is pedagogical)?**

- First and foremost, it is purely for my own learning. It is a hobby project
  after all.
- However, I would like to approach it in a way that it is approachable to
  others too. This will benefit me as well since it will consider future me
  too, who may forget about the mechanics after some time.

**Does this affect how you'd structure the bootstrap?**

>

---

# The Bootstrapping Decision

## On Self-Hosting

**How much does it matter to you that 0th "controls its own destiny" - that the
path from source to machine code is written in 0th?**

>

**Is there value in the bootstrap verification step (proving the compiler
compiles itself correctly), or is that more of a nice-to-have?**

>

**Does "self-hosting" mean "can compile itself" or "can extend itself"? The
proposed approach achieves the latter but not the former.**

>

---

## On the C99 VM

**Do you see the C99 VM as a stepping stone to discard, or as a permanent
development environment?**

>

**Does it bother you that the proposed approach makes C/assembly permanently
part of the system, rather than something you escape from?**

>

---

## On Practical Concerns

**How many architectures do you realistically want to support?**

- At least RISC-V, ARM, and x86, and of these, at least the 64 bit variants.
- Other architectures would be a nice-to-have, just to learn about them.
  - It would be a neat learning experience to know how older architectures
    worked out of curiosity and fascination of vintage/retro computers.

**If you only ever target RISC-V for bare-metal, does the "write a kernel per
platform" cost change?**

>

---

## On Failure and Pivoting

**What happens if you get stuck with one approach? Is it easy to pivot to the
other?**

>

**Which approach has more intermediate milestones where you have something
working?**

>

---

## A Hybrid Thought

**Could you do both? Start with hand-written kernels (simpler path to
bare-metal), then later write a code generator in 0th as an optional "level 2"
goal?**

>

---

# System Philosophy

## On Complexity

**Where would you rather spend your time - debugging a 0th-written code
generator, or writing/debugging multiple assembly kernels?**

>

**Which feels more tractable: one complex thing (code generator) or several
simple things (kernels)?**

>

**Does the "thin kernel" philosophy favor simpler hand-written kernels over a
complex 0th-written code generator? Or is the code generator "thin" because
it's temporary scaffolding?**

>

**How does "data-oriented design" apply to 0th's internal representation?
Should the VM/kernel be structured around data flow rather than abstractions?**

>

---

## On Integrated Environments

**Is bare-metal 0th just a "runtime," or do you aspire to a Cedar/Lisp Machine
style integrated environment (live patching, documentation explorer, graphics
in the prompt)?**

- I aspire to build an integrated environment with 0th.

**How important is the ability to introspect and modify the running system?
Does this favor one bootstrapping approach over the other?**

- Introspecting the running system seems very beneficial, especially when
  debugging. In terms of modifying though, if it degrades security or robustness
  in some areas, I'm happy to scale it down (or even disable it entirely).

---

## On OS/Security Model

**What's the isolation and security model for 0th? This affects what primitives
the kernel must provide.**

Options to consider:

1. **Monolithic with protection** - One kernel, MMU-based process isolation
   (like Unix)
2. **Microkernel** - Minimal kernel, services in userspace (like seL4)
3. **Exokernel** - Kernel provides protection only, no abstractions; apps
   implement their own resource management via library OSes (like MIT Exokernel)
4. **Hypervisor-based** - 0th as hypervisor, apps as lightweight VMs (like
   Qubes)
5. **Capability-based** - Fine-grained access control (like seL4 or CHERI)

>

**How much of the system should be "in 0th" vs "below 0th" (in the kernel)?**

>

**Does this choice favor one bootstrapping approach over the other?**

>

---

## On Concatenative Foundations

**Do you care about the mathematical purity of 0th (concatenative calculus,
compositional reasoning)? Or is Forth's pragmatic "untyped stack" sufficient?**

- I'd assume that the bootstrap portion would need to be written in "untyped"
  style to keep it as simple and close to the machine as possible. However, for
  the full-fledged 0th system (the userland), I highly prefer mathematical
  purity to make sense of writing more complex and high-level applications.

**Would you ever want typed stack effects? If so, does that affect how you
design the core?**

>

**Stack shuffling vs local variables: should 0th stay pure Forth, or consider
Kitten-style locals?**

- Kitten-style seems much more inviting and sensible. However, is this possible
  to also implement in the bootstrap portion? That I'll have to see...

---

## On "Data is Code"

**Both Forth and Lisp emphasize "data is code." Does the code generator
approach (where 0th manipulates its own representation) embody this better than
hand-written kernels?**

>

---

# Implementation

## On Debugging

**When something goes wrong, where would you rather be debugging - 0th code or
assembly/C?**

>

**Which approach produces a system you can understand end-to-end more easily?**

>

---

## On Prior Art

**There are many small Forths (jonesforth, sectorforth, lbForth). Should 0th
learn from them, or intentionally diverge to explore your own ideas?**

- Learning from them is okay, but since 0th is different from these Forths (and
  in fact wouldn't even be considered a Forth in the first place), it'll
  definitely diverge to explore my own ideas.

**Do you care about ANS Forth compatibility, or is 0th free to be its own
thing?**

- It should be free to be its own thing. However, the ANS Forth standard is an
  insightful source for learning about what is expected for a Forth. I can use
  this source as inspiration for 0th. Additionally, the discussions under each
  word give important context and room for thought on potential drawbacks or
  issues with how the word may be defined in the standard, which I'd have the
  freedom to address in 0th since I don't have to worry about
  standard-compliance.

---

# Open

## Other Thoughts

(Space for any other considerations that come up)

>
