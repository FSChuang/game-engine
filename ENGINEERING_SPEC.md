# Engineering Spec — Game Engine Project

**Audience:** any agent (human or AI) writing, reviewing, or refactoring code in this repository.
**Status:** binding. These are rules, not suggestions. When a rule and a request conflict, follow the rule and say so.

This document adapts Robert C. Martin's *Clean Code* principles to a **cross-platform C++17 game engine built on SDL3**, developed by a **team** across the semester. It is deliberately opinionated so that code written by different people (or different agents) reads as if one careful person wrote it.

---

## 0. Prime Directives

1. **Clarity over cleverness.** Code is read far more often than written. If a reader on the team cannot understand a construct in one pass, it is wrong — no matter how efficient or terse.
2. **Simplicity is the default.** Reduce complexity as much as possible (KISS). Do not add abstraction, configuration, or generality that no current requirement needs (YAGNI).
3. **Boy Scout Rule.** Leave every file you touch cleaner than you found it.
4. **Fix root causes.** When something breaks, find and fix the underlying cause, not the symptom.
5. **The engine/game boundary is sacred.** Reusable, game-agnostic code lives in the **engine**. Anything specific to one game lives in that game's code. Never leak game logic into the engine.

If a requested change would violate a Prime Directive, do not silently comply. State the conflict, then propose a compliant alternative.

---

## 1. What NOT to do (hard bans)

These produce exactly the "LLM-generated, hard-to-read" code this project rejects. They are non-negotiable.

- **No cryptic or over-engineered names.** No `mgr2`, `tmp`, `data2`, `doStuff`, `handleThing`, `MyClassImplHelperFactory`. A name must say what the thing *is* or *does*.
- **No speculative abstraction.** No interface, template, factory, or "manager of managers" introduced for a use case that does not exist yet. Add the abstraction the day the second concrete case appears — not before.
- **No deep nesting.** Maximum 3 levels of indentation inside a function. Beyond that, extract a function or use early returns.
- **No giant functions or classes.** See §4 and §5 for hard limits.
- **No magic numbers or magic strings.** Every literal with meaning becomes a named constant.
- **No commented-out code.** Delete it. Git is the history.
- **No dead code.** Unused functions, parameters, includes, and variables are removed, not kept "just in case."
- **No platform-specific code outside the platform layer.** No `#include <windows.h>`, no `__debugbreak()`, no POSIX-only calls scattered through engine logic. See §8.
- **No raw `new`/`delete` in ordinary code.** Ownership is expressed with smart pointers. See §6.
- **No redundant comments** that restate the code (`i++; // increment i`). See §7.

---

## 2. Naming

Names are the primary interface of the code. Spend effort here.

- **Descriptive and unambiguous.** `elapsedSeconds`, not `t`. `enemyVelocity`, not `ev`. The reader should not need to guess.
- **Pronounceable and searchable.** A name you cannot say out loud or grep for is a bad name. Avoid single letters except for trivial loop indices in a 2–3 line loop.
- **Meaningful distinctions.** If two names differ only by a number or noise word (`entity`, `entity2`, `entityData`, `entityInfo`), rename them to say how they actually differ.
- **No type/scope encoding in the name itself** (no Hungarian notation like `iCount`, `strName`). The one exception is the member/static *prefix* convention below, which this project keeps for consistency with its chosen style.
- **Avoid negative names.** Prefer `isVisible` over `isNotHidden`.

### Case conventions (match these exactly)

| Kind | Convention | Example |
|---|---|---|
| Types (class/struct/enum) | `PascalCase` | `class InputManager`, `struct Transform` |
| Public/member functions | `PascalCase` | `void OnUpdate(Timestep ts);` |
| Local variables & parameters | `camelCase` | `float deltaTime` |
| Private/protected member data | `m_` + `PascalCase` | `m_Position`, `m_IsRunning` |
| Static member data | `s_` + `PascalCase` | `s_Instance` |
| Constants / `constexpr` | `PascalCase` | `constexpr float DefaultGravity = 9.8f;` |
| Macros (avoid where possible) | `UPPER_SNAKE` with project prefix | `ENGINE_ASSERT(...)` |
| Namespaces | `PascalCase`, one project namespace | `namespace Engine { ... }` |
| Files | match the primary type | `InputManager.h`, `InputManager.cpp` |

Prefer named constants (`constexpr`) over macros. Reserve macros for things constants cannot express (assertions, conditional compilation).

---

## 3. Files, structure, and formatting

- **`#pragma once`** at the top of every header.
- **One primary type per file.** The file name matches that type. Small tightly-coupled helpers may share the file.
- **Include order**, grouped and blank-line separated: (1) the matching header, (2) other engine headers, (3) third-party (`<SDL3/SDL.h>`), (4) standard library. Include what you use; do not rely on transitive includes.
- **Vertical structure tells a story.** Order members `public:` → `protected:` → `private:`. Within a section, put the most important / highest-level functions first, and the helpers they call below them ("newspaper" order — read top to bottom, general to specific).
- **Declare variables close to first use**, not at the top of the function.
- **Keep related code vertically dense; separate unrelated concepts with one blank line.** Do not scatter one idea across the file.
- **Indentation:** tabs for indentation, consistent with the existing files. Never mix. Never break indentation to save a line.
- **No horizontal alignment** of consecutive assignments or declarations — it rots on the next edit and produces noisy diffs.
- **Keep lines reasonably short** (~120 cols). A very long line usually signals a missing intermediate variable or extracted function.
- **Braces on their own line for types and functions** (matching the project's existing style); braces always used even for single-statement `if`/`for`.

---

## 4. Functions

- **Small. Then smaller.** Target ≤ 20 lines. A function longer than ~40 lines must be justified in review or split.
- **Do one thing.** A function does one thing at one level of abstraction. If you can extract a meaningfully-named sub-function, the original was doing more than one thing.
- **Few arguments.** 0–2 is ideal, 3 is a ceiling. More than 3 → group them into a small struct/value object (see §9).
- **No flag arguments.** `SetScaling(bool proportional)` becomes two intention-revealing methods, or takes an `enum class ScalingMode`. A boolean parameter almost always means the function does two things.
- **No hidden side effects.** A function's name must predict everything it does. `IsColliding(a, b)` must not also move `a`. Query functions return; command functions act; don't mix.
- **Prefer early return** over nesting to handle boundary/guard conditions up front.
- **Command–Query Separation.** A method either changes state or answers a question, never both.

---

## 5. Types, objects, and data

- **Small classes, one responsibility (SRP).** A class has one reason to change. "Handles input and also audio" is two classes.
- **Few instance variables.** Many members is a smell that the class is doing too much.
- **Encapsulate.** Hide internal representation behind intention-revealing methods. Callers should not reach through an object into its internals (**Law of Demeter** — talk only to direct collaborators, no `a.GetB().GetC().DoThing()` chains).
- **Objects vs. data structures — pick one, don't build hybrids.** Either a class with behavior and hidden data, *or* a plain data struct with public fields and no behavior. A half-and-half type is a smell. Pure data carriers (e.g. `Vector2`, `Transform`) may be structs with public fields.
- **Base classes know nothing about derived classes.** No `if (type == Enemy)` in a base class. Prefer polymorphism over type-switching.
- **Prefer polymorphism to long `if/else`/`switch` on a type tag.** But do not build an inheritance hierarchy for a single case (§1).
- **Prefer value objects to raw primitives** where a primitive has invariants or units (§9).

### Entity model note (project-specific)

Milestone 1 keeps the entity type simple (position + renderable + update/render). **Design it so it can evolve toward a component / property-driven model later** (Milestone 3). Concretely: keep game-specific behavior *out* of the entity base type, and keep responsibilities separable, so composition can replace inheritance without a rewrite. Do not pre-build the component system now (§1), but do not paint yourself into an inheritance corner either.

---

## 6. Memory and ownership

- **Express ownership in the type.**
  - Unique ownership → `std::unique_ptr<T>`.
  - Shared ownership → `std::shared_ptr<T>` (only when ownership is genuinely shared).
  - Non-owning reference → a raw pointer or reference, understood to be observed, never deleted here.
- **No naked `new`/`delete`** in ordinary code. Use `std::make_unique` / `std::make_shared`.
- **RAII for every resource.** SDL handles (`SDL_Window`, `SDL_Renderer`, `SDL_Texture`) must be owned by a wrapper whose destructor releases them, so no path (including exceptions/early return) leaks. Never rely on remembering to call a `Destroy` function by hand.
- **Rule of Zero.** Design classes so the compiler-generated special members are correct. If you must write a destructor, revisit the design first.
- The project may define short ownership aliases (e.g. `Scope<T>` for unique, `Ref<T>` for shared) for readability — if used, use them **consistently** everywhere.

---

## 7. Comments

The best comment is the one you did not need to write because the code was clear.

- **Explain in code first.** Rename, extract, and introduce explanatory variables before reaching for a comment.
- **Good comments explain _why_,** not _what_: intent, a non-obvious trade-off, a warning of consequences, or a reference (e.g. why a specific SDL quirk is handled).
- **Ban:** redundant comments, obvious noise, closing-brace comments (`} // end for`), commented-out code, and journal/changelog comments (that's what Git is for).
- Every public engine API gets a **brief** doc comment: what it does, its parameters, and any precondition — no essays.
- `// TODO:` / `// FIXME:` are allowed but must name what and why, and should be tracked, not left to rot.

---

## 8. Cross-platform discipline (Mac / Windows / Linux)

This project is built and run on multiple operating systems by different teammates. Portability is a first-class requirement, not an afterthought.

- **SDL first.** For anything the platform provides (timing, filesystem, threads, input), use the SDL3 API, not the OS-native one. SDL exists precisely to abstract this.
- **No OS-specific headers or intrinsics in engine/game code.** If a truly platform-specific path is unavoidable, isolate it behind a common interface in a dedicated `Platform/` layer, and select the implementation at build time. The rest of the codebase stays platform-agnostic.
- **Portable assertions.** Do not use `__debugbreak()` (MSVC-only). Provide one cross-platform `ENGINE_ASSERT` macro that compiles out in release builds.
- **Filesystem:** never hard-code `\`. Use `/` or `std::filesystem`. Asset paths are relative to a defined root, identical on every OS.
- **Case-sensitive paths always.** File names in code must match on-disk names exactly, including case. (macOS is case-insensitive and will hide bugs that break instantly on Linux.)
- **Portable types.** Use fixed-width types (`uint32_t`, `int64_t`) where size matters; do not assume the size of `long` or the signedness of `char`.
- **CMake is the single source of truth for the build.** SDL3 is located via `find_package(SDL3)`. No absolute paths, no per-machine hacks committed. If it only builds on your machine, it is broken.

---

## 9. Domain modeling & boundaries

- **Value objects over bare primitives** when a value has units or invariants: `Timestep` wrapping seconds, `Vector2`, an `enum class ScalingMode { Pixel, Proportional }` instead of a `bool`. This makes illegal states unrepresentable and reads better at call sites.
- **Keep configurable data at the top level.** Gravity strength, window size, frame rate, and similar tunables are passed in / configured, never hard-coded deep inside a system. (Directly satisfies the "configurable gravity" requirement — expose `SetGravity(float)`, don't bury a constant.)
- **Dependency injection over global reach-in.** A system receives its collaborators (e.g. a renderer, a config) rather than fetching them from a global. Minimize singletons; if one exists (e.g. a single `Application`), it is the rare deliberate exception, documented as such.
- **Encapsulate boundary conditions** (edge indices, min/max, wrap-around for animation frames) in one place with a clear name, rather than repeating `+1`/`-1` arithmetic across the code.

---

## 10. Testing

Tests are part of the deliverable, not an extra. Untested engine systems are treated as incomplete.

- **Engine systems must be unit-testable in isolation.** This is a design constraint: if physics or collision cannot be tested without opening a window, the coupling is wrong — separate the logic from the rendering/SDL layer.
- **F.I.R.S.T.** Tests are **Fast**, **Independent** (no shared mutable state, any order), **Repeatable** (same result every run, no reliance on timing/rng/network), **Self-validating** (a clear pass/fail, no manual eyeballing), and written **Timely** (alongside the code, not "later").
- **One logical assertion per test.** A test checks one behavior. Its name states that behavior: `Collision_ReturnsTrue_WhenBoxesOverlap`.
- **Readable tests** follow Arrange–Act–Assert. Test code is held to the same cleanliness bar as production code.
- **Deterministic engine logic.** Update logic takes an explicit `Timestep`/delta so it can be driven with fixed values in tests. No hidden reads of the wall clock inside logic.
- Cover at minimum: physics integration & configurable gravity, collision detection (overlap, touching, disjoint), input abstraction, and scaling-mode conversion.

---

## 11. Team workflow & version control

- **Consistency beats personal preference.** Match the conventions already in the file and in this document, even if you would personally do it differently. A uniform codebase is worth more than any individual's style.
- **Small, focused commits** with imperative messages that say *why* (`Add configurable gravity to PhysicsSystem`), not `wip` or `fixes`.
- **No commented-out code, no dead files, no build artifacts** committed. A `.gitignore` covers `build/`, binaries, and IDE files. A `.gitattributes` normalizes line endings (LF) so Windows/macOS/Linux teammates don't fight CRLF.
- **The `main`/shared branch always builds and passes tests on all target platforms.** Broken shared code blocks the whole team — do not push it.
- **Review against this spec.** A change that violates a rule here is requested to change, regardless of who wrote it.

---

## 12. Code smells — reject on sight

If you are writing or reviewing code and notice any of these, stop and fix the design:

- **Rigidity** — a small change forces a cascade of edits elsewhere.
- **Fragility** — one change breaks unrelated things.
- **Immobility** — a useful piece can't be reused because it's tangled with unrelated concerns.
- **Needless complexity** — machinery for requirements that don't exist.
- **Needless repetition** — the same logic copy-pasted instead of factored into one place (DRY).
- **Opacity** — the code is hard to understand on first read.

---

## Agent checklist (run before proposing any code)

- [ ] Names are descriptive, pronounceable, and consistent with §2.
- [ ] Functions are small, do one thing, ≤ 3 args, no flag args, no hidden side effects.
- [ ] Classes are small, single-responsibility, encapsulated, no Demeter violations.
- [ ] No magic numbers/strings; tunables are configurable and top-level.
- [ ] No naked `new`/`delete`; ownership is explicit; SDL handles are RAII-wrapped.
- [ ] No platform-specific code outside `Platform/`; paths portable & case-correct.
- [ ] No dead code, no commented-out code, no redundant comments.
- [ ] Nesting ≤ 3; guard clauses used; no needless complexity or repetition.
- [ ] Engine logic is decoupled from SDL enough to unit-test; tests included and F.I.R.S.T.
- [ ] The engine/game boundary is respected — no game logic in the engine.
- [ ] It builds via CMake and would build on a teammate's OS, not just this machine.

> When unsure, choose the simpler, clearer option and leave a short note on the trade-off. Do not invent complexity to look thorough.
