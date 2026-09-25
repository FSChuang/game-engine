# Build & Run

## Configure, build, test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

This is the exact sequence `.github/workflows/ci.yml` runs on every push.

!!! note
    This repository builds the reusable **Engine** and its **test suite**
    only — there is no runnable game target here. A real game (for example
    the *Spare Parts* course project) links `Engine` as a dependency from its
    own, separate repository. See the
    [Engine/Game boundary](../architecture/overview.md#engine-game-boundary)
    for why that split is structural, not incidental.

## What gets built

| Target | Kind | Always built? |
|---|---|---|
| `Engine` | Static library | Yes |
| `EngineNetworkCore` | Static library | Yes |
| `EngineNetwork` | Static library | Only if cppzmq is found (see [Installation](installation.md)) |
| Unit tests (see below) | Executables, CTest-registered | Yes, unless configured with `-DBUILD_TESTS=OFF` |

## Tests

`ctest` currently runs these registered suites:

- `CoreTests`, `EntityTests`, `PhysicsSystemTests`, `CollisionTests`, `ScalingTests`, `TimelineTests` — link only `Engine`
- `NetworkProtocolTests`, `PlayerRegistryTests`, `ServerDispatchTests` — link only `EngineNetworkCore` (build regardless of ZeroMQ availability)
- `SocketPubSubTests` — links `EngineNetwork`; only built (and only registered with CTest) when `EngineNetwork` exists

A few additional executables build alongside the tests **only when
`EngineNetwork` exists**, but are deliberately *not* CTest-registered because
they drive real multi-process ZeroMQ scenarios rather than a single
deterministic unit: `NetworkSmokeServer`/`NetworkSmokeClient` (a manual
REQ/REP smoke check) and `HeadlessServer`/`TestClient` (a manual multi-client
integration harness). Run these directly from `build/bin/` if you need to
exercise them.

## Toggling tests off

```bash
cmake -S . -B build -DBUILD_TESTS=OFF
```

For the full repository layout and rationale (why `Engine/` is a separate
static library, why tests live where they do), see
[`PROJECT_STRUCTURE.md`](https://github.com/FSChuang/game-engine/blob/main/PROJECT_STRUCTURE.md)
in the repository root — this page intentionally doesn't repeat it.
