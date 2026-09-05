# Game Engine

[![Build and Tests](https://github.com/FSChuang/game-engine/actions/workflows/ci.yml/badge.svg)](https://github.com/FSChuang/game-engine/actions/workflows/ci.yml)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![SDL3](https://img.shields.io/badge/SDL-3-green.svg)
![CMake](https://img.shields.io/badge/CMake-3.16%2B-informational.svg)

A cross-platform 2D game engine built on **SDL3** in **C++17**, developed as a
team over the semester. The engine is the reusable core; each teammate builds
their own game on top of it.

## Architecture

- `Engine/` contains reusable, game-agnostic systems.
- `Game/` contains individual game state, entity choices, controls, patrol behavior, and collision responses.
- `Application` exposes a small update/render callback seam so `Game` can participate in the engine loop without putting gameplay logic inside `Engine`.

## Build & run

```bash
mkdir build && cd build
cmake ..
make
./bin/main
```

## Requirements

- CMake 3.16+
- A C++17 compiler
- SDL3  (`brew install sdl3` on macOS / `sudo apt install libsdl3-dev` on Ubuntu)

## Milestone 1 Engine Task Mapping

| Task | Engine implementation | Design / responsibility |
|---|---|---|
| Task 1 – Core Graphics Setup | `Engine/Core/Application`, `Engine/Renderer/Renderer` | SDL initialization/lifecycle, 1920x1080 window and renderer, main loop, blue frame clear/render lifecycle |
| Task 2 – Generic Entity System | `Engine/Entity/Entity` | Generic entity state (position, size, color, velocity); engine rendering works without knowing whether the entity is a player, enemy, platform, etc. |
| Task 3 – Physics System | `Engine/Physics/PhysicsSystem` | Configurable gravity; updates velocity and position using delta time |
| Task 4 – Input Handling | `Engine/Input/InputManager` | Keyboard polling through `SDL_GetKeyboardState`; game code queries abstract key state rather than implementing gameplay through SDL key events |
| Task 5 – Collision Detection | `Engine/Collision/Collision` | Generic AABB overlap detection between entities; returns whether two entities overlap; collision response remains game-specific |
| Task 6 – Scaling System | `Engine/Renderer/Renderer`, `Engine/Core/Application` (input-triggered toggle) | Constant (pixel-based) and Proportional (percentage-based) scaling; Tab toggles modes through the input system |
