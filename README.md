# Game Engine

[![Build and Tests](https://github.com/FSChuang/game-engine/actions/workflows/ci.yml/badge.svg)](https://github.com/FSChuang/game-engine/actions/workflows/ci.yml)
[![Deploy Documentation](https://github.com/FSChuang/game-engine/actions/workflows/docs.yml/badge.svg)](https://github.com/FSChuang/game-engine/actions/workflows/docs.yml)

![C++17](https://img.shields.io/badge/C%2B%2B-17-4F5D3D?style=flat-square&labelColor=3B342C)
![SDL3](https://img.shields.io/badge/SDL-3-8E5936?style=flat-square&labelColor=3B342C)
![CMake](https://img.shields.io/badge/CMake-Build-5F6B47?style=flat-square&labelColor=3B342C)
![Platforms](https://img.shields.io/badge/Platforms-macOS_%C2%B7_Linux-876B44?style=flat-square&labelColor=3B342C)
[![API Reference](https://img.shields.io/badge/API_Reference-Doxygen-685F52?style=flat-square&labelColor=3B342C)](https://fschuang.github.io/game-engine/reference/application/)

**[Documentation](https://fschuang.github.io/game-engine/)**

A cross-platform 2D game engine built on **SDL3** in **C++17**, developed as a
course project. This repository contains only the reusable engine — a
consuming game lives in its own separate repository and links this one as a
dependency; see [Example Project](#example-project) below.

## Features

- **Application** — SDL lifecycle and the main update/render loop
- **Renderer** — window and drawing, with constant and proportional scaling modes
- **Entity** — a generic position/size/color/velocity object, no behavior
- **Physics** — configurable gravity via semi-implicit Euler integration
- **Input** — polled keyboard state, held vs. just-pressed
- **Collision** — strict axis-aligned bounding-box overlap
- **Timeline** — logical time (pause, scale, tic size), independent of the wall clock, with parent/child composition
- **Networking** — an explicit wire protocol (encode/decode), server-side player bookkeeping and request/reply dispatch, and an optional ZeroMQ `Socket` wrapper (REQ/REP and PUB/SUB)

## Repository Structure

```
Engine/     the reusable engine (static library, plus two networking targets)
Tests/      engine unit tests, run via ctest
docs/       the MkDocs Material documentation site
scripts/    documentation tooling (Doxygen API-fragment generator)
```

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Test

```bash
ctest --test-dir build --output-on-failure
```

## Dependencies

- CMake 3.16+
- A C++17 compiler
- SDL3 — required to build the `Engine` target
- cppzmq — optional, only needed for the `EngineNetwork` transport target

See the [Installation guide](https://fschuang.github.io/game-engine/getting-started/installation/)
for exact per-platform install commands.

## Documentation

The [documentation site](https://fschuang.github.io/game-engine/) is the
canonical place to learn how this engine works: Getting Started, Concepts,
Systems, Guides, API Reference, Architecture, and a worked example. This
README stays intentionally short — it isn't duplicated here.

API reference facts (signatures, types, enum values) are generated directly
from the public C++ headers via Doxygen, so they can't drift out of sync with
the actual code; the surrounding explanation is still written by hand.

## Example Project

**[Spare Parts](https://github.com/FSChuang/spare-parts)** is a separate
repository that links this engine to build an actual game. It demonstrates
one way to compose these systems — including a client/server networking
topology built on the engine's networking primitives — but its game-specific
code (`NetworkClient`, `PeerClient`, gameplay rules, and so on) is not part of
the reusable Engine API.

## Project Status

The engine currently includes the systems listed above and is under active
development as part of a game-engine course project.
