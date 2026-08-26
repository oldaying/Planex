# ADR-0004: Implementation language is C17, not Rust / Zig / C++

## Status

Accepted

Date: 2026-08-24

## Context

Planex is implemented in C17 with zero external dependencies. This is unusual for a 2020s research-grade UI library — Rust, Zig, and even modern C++23 are all more popular choices for new systems programming projects. The choice of C requires justification.

The forces at play:

1. **Abstraction purity test:** If Planex's three abstractions (Relation + Estimate + Closure) are real, they should be expressible in any language. The weakest proof is "we needed Rust's borrow checker to make this safe"; the strongest proof is "we did it in C without any language-level help."
2. **Embeddability:** Planex targets, among other things, embedded systems. C is the lingua franca of embedded; Rust and Zig are still rare in deeply constrained environments.
3. **Auditability:** Research-grade projects must be readable by skeptics. C has the lowest "syntax friction" of any systems language — anyone who has programmed can read C, even if they can't write it well.
4. **Dependency minimalism:** The README boasts "zero external dependencies". This is achievable in C; it's nearly impossible in Rust (Cargo pulls in hundreds of crates for basic features) or Zig (still maturing stdlib).

The forces pushing *against* C are well known: no memory safety, no RAII, no algebraic data types, no module system, manual error handling. These are real costs.

## Decision

**Planex is implemented in C17, with zero external dependencies beyond libc and optional FreeType/fontconfig for CJK fonts.**

The choice is principled, not nostalgic. Three reasons:

1. **Abstraction purity test.** Conal Elliott's denotative design philosophy says: if an abstraction is real, it should be expressible in the thinnest possible runtime. C is the thinnest mainstream runtime. If Relation/Estimate/Closure work in C, they're real abstractions, not language-parasites.
2. **Embeddability.** Planex can be compiled for STM32, ESP32, and other constrained targets without modification. Rust and Zig have weaker stories here.
3. **Auditability.** A skeptical researcher who wants to verify Planex's claims can read C. They don't need to learn Rust's lifetime elision rules, Zig's comptime, or C++'s template instantiation model.

## Consequences

### Positive
- The abstractions are proven language-independent. If they work in C, they'll work in any language.
- Planex is embeddable in environments Rust/Zig cannot reach (deeply constrained MCUs, legacy RTOSes).
- Zero dependencies is achievable. The full library is ~11K lines of C, builds in seconds, and ships a single `.a` file.
- C is stable. Code written in C17 today will compile in 2035. Rust and Zig are still evolving their language semantics.

### Negative
- No memory safety. We accept the cost of careful manual review.
- No RAII. Cleanup must be explicit (`px_*_free`). Users can leak.
- No algebraic data types. The `px_intent_kind` enum + tagged union is hand-rolled.
- No module system. Header hygiene is manual.
- Some contributors will refuse to touch C. We accept losing them.

### Neutral
- The choice is irreversible without a rewrite. If Planex 2.0 wants to be Rust, that's a full reimplementation.

## Alternatives Considered

### Alternative 1: Rust
- **What:** Rust with `no_std` for embedded targets. Cargo for dependency management.
- **Why rejected:**
  - Borrow checker adds nothing to Planex's abstractions. The Relation graph is acyclic and doesn't need lifetime analysis.
  - `no_std` is restrictive — many core Rust features are unavailable. We'd be writing "C in Rust syntax".
  - Cargo makes "zero dependencies" impossible. Even an empty Rust project pulls in `core`, `compiler_builtins`, and a few dozen transitive crates.
  - Rust's compile times would slow the iterate-fast research loop.

### Alternative 2: Zig
- **What:** Zig is the trendy choice for new systems projects. Manual memory management, comptime, no hidden control flow.
- **Why rejected:**
  - Zig is still pre-1.0. Language semantics have changed between minor versions. A research-grade project should not depend on a moving target.
  - Zig's stdlib is in flux. What works in 0.13 may not work in 0.14.
  - Zig's embedded story is improving but not as mature as C's.
  - The "comptime" feature is nice but doesn't help Planex's abstractions. We'd be paying Zig's instability cost for no benefit.

### Alternative 3: C++23
- **What:** Modern C++ with concepts, ranges, modules, smart pointers.
- **Why rejected:**
  - C++ adds hidden complexity (constructors, RAII, template instantiation) that obscures the abstractions. The point of Planex is that the abstractions are visible; C++ hides them.
  - Build systems for C++ are a mess (CMake + vcpkg + Conan). Planex's current CMake + zero deps is dramatically simpler.
  - "Modern C++" is a moving target. C++17 vs C++20 vs C++23 fragmentation is real.

### Alternative 4: Hybrid — core in C, bindings in other languages
- **What:** Planex core stays C, but we ship Rust / Zig / Python bindings.
- **Why rejected for now:** Not rejected in principle — but deferred. Until the abstractions are proven (see ADR-0001, ADR-0002), bindings would lock in a possibly-wrong API. Bindings come after stability, not before.

## References

- Code: `CMakeLists.txt` — `set(CMAKE_C_STANDARD 17)`
- Code: `include/planex/*.h` — public API, all C
- Related docs: `docs/faq.md` — "Why C? Why not Rust/Zig/C++?" question
- External: Conal Elliott, "Denotational Design with Type Class Morphisms" — the abstraction-purity motivation
- External: Andrew Kelley, "Why Zig Isn't 1.0 Yet" — similar discipline from a sibling project
- External: seL4 — also a C-based research-grade project that justifies the language choice carefully
