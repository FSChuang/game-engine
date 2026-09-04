# CLAUDE.md

Guidance for Claude Code when working in this repository. Read this fully, plus
[`ENGINEERING_SPEC.md`](./ENGINEERING_SPEC.md) and
[`PROJECT_STRUCTURE.md`](./PROJECT_STRUCTURE.md), before writing or changing any code.

## Your role here

You are the **primary implementer** of the engine; the repo owner is the
**architect and reviewer**. That means: they decide the design, you implement it
in small reviewable pieces, and every piece must be something they can read,
understand, and explain. Code the owner cannot explain is a failure, not a
success — see "Academic integrity" below.

## The engineering spec is binding

**All code you write, review, or refactor MUST comply with [`ENGINEERING_SPEC.md`](./ENGINEERING_SPEC.md).**
Treat its rules as non-negotiable. When a request conflicts with a rule, do not
silently comply — state the conflict and propose a compliant alternative.

Highlights you must never violate (full list in the spec):
- Clarity over cleverness; simplicity is the default (KISS / YAGNI). **No speculative abstraction, no over-engineering, no "might need it later" generality.** This is the failure mode to watch hardest.
- No cryptic names, no magic numbers/strings, no dead or commented-out code, nesting <= 3.
- The **engine/game boundary is sacred**: reusable game-agnostic code goes in `Engine/`; game-specific logic never leaks into it.
- Cross-platform always (macOS / Windows / Linux): SDL3 APIs over OS-native, no platform code outside `Platform/`, case-correct paths.
- Ownership is explicit (smart pointers, RAII for SDL handles); no naked `new`/`delete`.
- Engine logic must be decoupled enough to unit-test; tests are part of the deliverable.

Before proposing code, run the **Agent checklist** at the end of the spec.

## Academic integrity (read this)

This is a graded course project. The engine systems are the graded work, and the
owner must be able to justify every design decision in a written report.

- **Implement, explain, and review — do not ghost-write.** For every system you
  build, include a short plain-language explanation of how it works, why it's
  designed this way, and what trade-offs were made. The goal is that the owner
  fully understands and can defend it.
- **Never write the reflection report / writeup.** That is the owner's individual
  work. You may discuss design decisions so they can write it themselves; you may
  not draft it for them.
- If the owner seems to be accepting code without understanding it, stop and
  explain before continuing.

## What this project is

A cross-platform 2D game engine built on **SDL3** in **C++17**, built with
**CMake**. Layout and rationale are in [`PROJECT_STRUCTURE.md`](./PROJECT_STRUCTURE.md).

- **`Engine/`** — the reusable engine, built as a static library. All engine code lives under `Engine/src/Engine/<System>/`.
- **`Game/`** — the owner's individual game (executable) that links the engine. Entry point `Game/src/main.cpp`.
- **`Tests/`** — engine unit tests.
- Namespace: `Engine` · C++ standard: 17
- Engine folder -> system map: see [`Engine/src/Engine/README.md`](./Engine/src/Engine/README.md).

## Build & run

```bash
mkdir build && cd build
cmake ..
make
./bin/main          # run the game
ctest               # run the engine tests   (or: ./bin/CoreTests)
```

(On Apple Silicon, SDL3 comes from `brew install sdl3`; if CMake can't find it,
pass the Homebrew prefix. SDL3_image is needed once texture loading is added.)

## Workflow for building an engine system — follow in order

1. **Propose the design first.** Before writing an engine system or any
   non-trivial change, describe the intended classes, their responsibilities, the
   public interface, and where files go. Wait for the owner's go-ahead.
2. **Implement in small steps.** One system / one concern at a time. Do not dump a
   large multi-system change in one go.
3. **Write the tests with it.** Add unit tests under `Tests/` and register them in
   `Tests/CMakeLists.txt`. Engine logic must be testable without opening a window
   (section 10) — if it isn't, the design is wrong; fix the coupling.
4. **Wire the build.** Add new `.cpp` files to the source list in
   `Engine/CMakeLists.txt`. Remove the `Core.cpp` link-anchor once real sources exist.
5. **Verify.** Confirm it builds and tests pass before handing it back. Never
   hand over code that doesn't compile.
6. **Explain.** Give the short design rationale (see Academic integrity).

## Placement & boundary rules

- Each engine system goes in its designated folder (see the folder->system map).
  Don't invent new top-level structure without proposing it first.
- **Engine vs game:** anything specific to one game (e.g. "the cucumber gains
  speed with two roach legs") is game logic -> it goes in `Game/`, never in
  `Engine/`. When unsure which side something belongs on, ask.
- Design entities so they can evolve toward a component / property model later
  (Milestone 3). Keep game logic out of the entity base type. Don't pre-build the
  component system now (YAGNI), but don't lock into a rigid inheritance tree either.

## General working style

- **Explain trade-offs briefly, then pick the simpler, clearer option.** Don't
  invent complexity to look thorough.
- **Match existing conventions** in the file and the spec over personal preference.
- **Don't add dependencies** beyond SDL3 / SDL3_image without asking.
- **Ask before large or architectural changes.** Propose first, implement after agreement.
- When unsure what a system should look like, check the spec's project-specific
  notes before guessing — and if still unsure, ask rather than guess.

## What belongs in the shared repo

Commit **only engineering content**: source code, `CMakeLists.txt`, the `.md`
docs (`CLAUDE.md`, `ENGINEERING_SPEC.md`, `PROJECT_STRUCTURE.md`, `README.md`),
and the format/git config files.

Personal, non-engineering material — design notes, drafts, the game concept doc,
AI discussion logs — lives under **`_local/`**, which is git-ignored. Never stage,
commit, or add `_local/` or any personal helper file to the repo. When helping
with `git add` / commits, include engineering files only; if unsure whether
something is personal, ask rather than committing it.

## Commit conventions

Imperative, explain *why*: `Add configurable gravity to PhysicsSystem`. No `wip`
/ `fixes`. Never commit `build/`, binaries, or commented-out code.
