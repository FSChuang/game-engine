# Project Structure

A three-part layout that keeps the reusable engine, individual games, and tests
cleanly separated — the foundation we build the whole semester on.

```
.
├── Engine/                 # Reusable, game-agnostic engine (TEAM-owned, built as a static library)
│   ├── CMakeLists.txt
│   └── src/Engine/
│       ├── Core/           # Plumbing, app/game-loop, time        (Task 1)
│       ├── Renderer/       # Window + rendering, scaling          (Task 1, 6)
│       ├── Entity/         # Generic game-object system           (Task 2)
│       ├── Physics/        # Physics + configurable gravity       (Task 3)
│       ├── Input/          # Keyboard abstraction                 (Task 4)
│       ├── Collision/      # Bounding-box collision               (Task 5)
│       └── Platform/       # Isolated OS-specific code (§8)
│
├── Game/                   # An individual game built ON the engine (YOURS)
│   ├── CMakeLists.txt
│   ├── src/main.cpp        # entry point; uses engine systems
│   └── assets/             # this game's sprites, etc.
│
├── Tests/                  # Engine unit tests (§10)
│   ├── CMakeLists.txt
│   └── ExampleTest.cpp
│
├── media/                  # Images for the README
├── _local/                 # Personal notes/drafts — git-IGNORED, never pushed
│
├── CMakeLists.txt          # Top-level: wires Engine + Game + Tests
├── ENGINEERING_SPEC.md     # Binding engineering rules
├── CLAUDE.md               # Instructions for Claude Code
├── .clang-format .gitignore .gitattributes
└── PROJECT_STRUCTURE.md    # this file
```

## Why the engine is a separate library

Building `Engine/` as a static library means every teammate's game links the
*same* engine, and reusable logic physically cannot hide inside one person's
game. This enforces the engine/game boundary (ENGINEERING_SPEC.md §0, §5)
structurally, not just by convention.

## Build

```bash
mkdir build && cd build
cmake ..
make
./bin/main        # run your game
ctest             # run the engine tests   (or: ./bin/CoreTests)
```

## Adding a new engine system

1. Create `Engine/src/Engine/<System>/<System>.h` + `.cpp`.
2. Add the `.cpp` to the source list in `Engine/CMakeLists.txt`.
3. Add a test under `Tests/` and register it in `Tests/CMakeLists.txt`.
4. Use it from your game via `#include "Engine/<System>/<System>.h"`.
