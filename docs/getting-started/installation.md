# Installation

## Prerequisites

- **CMake 3.16+**
- **A C++17 compiler**
- **SDL3** — required to build the `Engine` target at all.

!!! warning "Tested environment"
    The only environment continuously verified by CI is **Ubuntu Linux**
    (see `.github/workflows/ci.yml`, which installs SDL3 and ZeroMQ via
    `apt` and builds on `ubuntu-latest`). macOS install steps below are
    documented and used by contributors, but are not CI-checked. Windows is
    a stated cross-platform goal — SDL3 exists precisely to abstract the
    platform layer — but this repository does not yet document Windows
    install steps or run Windows CI.

### macOS

```bash
brew install sdl3
```

### Ubuntu / Linux

```bash
sudo apt install libsdl3-dev
```

## Optional: ZeroMQ, for networking

Everything under `Engine/src/Engine/Network/` is split across **two** CMake
targets with different dependency requirements — see
[Build & Run](build-and-run.md) for what each one produces.

- **`EngineNetworkCore`** (protocol encode/decode, the player registry, pure
  dispatch logic) is plain C++17 — it needs nothing beyond the standard
  library and builds unconditionally.
- **`EngineNetwork`** (the `Socket` transport wrapper) additionally needs
  **cppzmq** (which pulls in **libzmq**). If cppzmq isn't found, CMake
  configuration does not fail — it skips the `EngineNetwork` target and
  prints a `message(STATUS ...)` explaining why, so the rest of the engine
  is unaffected.

```bash
# macOS
brew install cppzmq

# Ubuntu / Linux
sudo apt install libzmq3-dev cppzmq-dev
```

## Target availability

| Target | Requires | Availability |
|---|---|---|
| `Engine` | SDL3 | Always required — configuration fails without it (`find_package(SDL3 REQUIRED)`) |
| `EngineNetworkCore` | C++17 standard library only | Always available, regardless of SDL3 or ZeroMQ |
| `EngineNetwork` | cppzmq → libzmq | Optional — silently skipped if cppzmq isn't found |

Next: [Build & Run](build-and-run.md).
