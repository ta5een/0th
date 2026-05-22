# Notes

## 2026-02-04

### Over the Shoulder 1: Text Preprocessing in Forth

- A practical example of developing DSL to HTML parser in Forth.
- They use a top-to-bottom approach in developing the program. I'm used to a bottom-to-top approach, so this is interesting.
- They admit that they wish GForth had a powerful REPL environment like LISP and Smalltalk, but they were able to get around this by defining a metaprogram with the word "ecr" for "edit-compile-run"
- They have a strict rule of "no more than two lines per definition".
- I like how they defined `: interpret either& or< or> ( else ) emit ;` to check for characters that need to be escaped, otherwise they are just emitted to output.
- They showcased a technique that they dubbed "similar to Scheme's continuations". They mentioned this is possible due to how Forth exposes the call stack to the program.
- They reiterate that redefining existing words based on context is okay, and in fact encouraged.
- They express that in Forth, much like in Lisp, "data is code".
- They say, "You see a pattern here? […] I code in a very literate way, a very conversational way. There is no emphasis on functions. There is no emphasis on data structures. There is no emphasis on abstraction for abstraction sake."
- Just saw how their pattern of reusing words backfired! They redefined `start` a couple times in their code, and `process` accidentally used the wrong definition. Though, they quickly realised this and moved the correct `start` definition before `process`, resolving the bug.

## 2026-02-03

### Computer History Museum - Eric Bier Demonstrates Cedar

- Demonstrated window system is a tile-based
- Showcased how development is done in this machine with the Mesa programming language
- Mesa is heavily inspired by Pascal, with a strong type system
- Cedar system heavily relies on a three-button mouse. You use different combinations of Control, Shift, and mouse buttons to perform contextual actions like copying and pasting text.
- Tioga, the native editor, supports documentation and code editing. In fact, Cedar blurs the two, making it possible to, for example, insert graphics into code to illustrate mathematical calculations.

### Symbolics Lisp Machines (Talk and Demo)

- Overview of the history of LISP machines, and a demo of an x86_64 implementation of the Symbolics Lisp Machine running in a virtual machine.
- It's a mix of a GUI and CLI interface… pretty cool.
- Interact with mouse and keyboard shortcuts.
- Zmacs -- makes sense, since the metaprogramming language of Emacs is LISP!
- Autocomplete of commands and arguments.
- Document examiner: interactive documentation explorer with hierarchy graphic.
- Selection of text kinda reminds me of Xerox's Cedar.
- Demoed an example of triggering a debugger, fixing a bug, and compiling the code.
- Patch system in spirit of Git, though they mentioned a "versioned file system".
- "The Symbolics Lisp Machine allowed multiple lisps syntaxes to co-exist, even ones that use the same package name for different semantics".
- Prompt can output graphics (not restricted to just text).

### Jon Purdy - Concatenative Programming: From Ivory to Metal

- Jon Purdy is the creator of the Kitten language
- They gave a brief history of concatenative languages, and summarised that Turing machines inspired imperative languages, lambda calculus inspired functional languages, and combinatory logic is largely applicable to concatenative languages.
- They express that concatenative languages allow for point-free programming, allowing expressions to be refactored and expressed in a more literate way.
- Many languages are "value-level", where semantics and state are too closely coupled. You can't easily factor out subexpressions without writing "wrapper" code. Also, you can't reason about subparts of programs without context (state and history).
- Combinatory logic allows for substructural type systems (e.g. Rust's affine system for tracking mutable references).
- Concatenative Calculus: "all expressions denote functions […] juxtaposition […] denotes function composition".
- Most concatenative languages use a heterogeneous stack, though other models have been proposed. Still, the stack is convenient and efficient in practice.
- Forth is both "imperative" and "pure". Forth is inherently untyped, with stack effects comments more of a convention.
- Stack shuffling (e.g. `swap`, `drop`) is natural in Forth, though it is difficult to follow for humans. Local variables like in Kitty solve this by modelling them as lambda expressions. "'Lambda' is decoupled into 'anonymous function' and 'variable binding'".
- Pure concatenative code can be translated to lambda calculus
- Languages can be concatenative at the lexical level (Forth, Factor) or the term level (Kitten).
- Value proposition of concatenative programming:
  - Pure functions are a good default unit of behaviour
  - Function composition is a good default means of combining behaviours
  - Juxtaposition is a convenient notation for composition
  - Having a simple language with strong mathematical foundation makes it easier to develop tooling and reason about code.
- To read: "Implementation of Stack-based Languages on Register Machines (1996) M. Anton Ertl"
- Type systems in concatenative languages:
  - Mostly dynamically typed (Joy, Factor, PostScript) or untyped (Forth)
  - Cat was the first concatenative language with Hinley-Milner type system
  - Most simplest "type system": count the stack arity of each word
    - Though some words are polymorphic -- could be resolved via Higher-Rank Polymorphism
  - Algebraic Effects: used in Kitten, inspired by Koka.
    - In Kitten, effect types are compositional and polymorphic

## 2026-01-31

### Casey Muratori - The Thirty Million Line Problem

- Modern software (particularly operating systems) are way too complex due to excessive code.
- Why not return to simple hardware interface standards and reduce intermediate software layers?
- Lack of Direct Hardware Access: Today, hardware is hidden behind complex drivers. Modern drivers are often buggy, proprietary, and impossible to debug.
- Complexity Spiral: As software grows to tens of millions of lines, it becomes impossible for any single human to understand.
- For Linux specifically, even though no system would use every single driver defined in the kernel, one cannot understand the system confidently without understanding all the complex driver code, since these may affect other unrelated systems.
- Thin Kernels: The operating system should return to being a "resource multiplexer" rather than a massive library of features. It should stay out of the way and let the application talk to the hardware as directly as possible.
- "Data-Oriented" design: Focus on how data flows through the CPU and memory, rather than building deep, complex object hierarchies that thrash the cache.
- Modern machines are capable of processing gigabytes of data per second, yet a simple text editor often lags when typing a single character.
  - This isn't a hardware limitation, but a software catastrophe caused the cumulative weight of these 30 million lines.

### Casey Muratori - The Big OOPs: Anatomy of a Thirty-five-year Mistake

- Argued that the prevalant approach of OOP encapsulation that directly mirrors the real world is an error in software architecture.
- He argues this approach makes code difficult to manage, refactor, and scale for complex problems.
- Alternative: Entity Component System (ECS) - better for performance and managing complexity, since it doesn't encourage needless abstractions.
- He gave a lot of context of where this flawed approach came from, including the history of C++, Smalltalk, Simula. Very insightful and gives clarity of why C++ is the way it is now.
- C++ could have had discriminative unions from day one!
- Placing boundaries around systems and functions (verbs) rather than data structures (nouns) can often lead to more maintainable and efficient code
