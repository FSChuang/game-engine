# Engine source layout

Each folder is one engine subsystem. Included via `#include "Engine/<Folder>/<File>.h"`
(the `Engine/src` dir is on the include path). Keep everything here **game-agnostic** —
game-specific logic belongs in the individual game's repository (e.g. `spare-parts`),
never in the engine (ENGINEERING_SPEC.md §0 & §5).

| Folder       | Responsibility                                   | Milestone 1 Task |
|--------------|--------------------------------------------------|------------------|
| `Core/`      | Shared plumbing: ownership aliases, assert, app/game-loop, time | Task 1 |
| `Renderer/`  | Window + rendering wrappers; scaling modes       | Task 1, Task 6   |
| `Entity/`    | Generic game-object system (position + renderable)| Task 2          |
| `Physics/`   | Physics + configurable gravity                    | Task 3          |
| `Input/`     | Keyboard-state abstraction (`IsKeyPressed`)       | Task 4          |
| `Collision/` | Bounding-box collision detection                  | Task 5          |
| `Platform/`  | Any unavoidable OS-specific code, isolated here (§8) | —             |

## As you build a system

1. Add its `.h`/`.cpp` under the right folder.
2. Add the `.cpp` to the source list in `Engine/CMakeLists.txt`.
3. Add a unit test under `Tests/` (§10 — engine logic must be testable without a window).

Design entities so they can evolve toward a component / property model later
(Milestone 3) — keep game logic out of the entity base type (ENGINEERING_SPEC.md §5).
