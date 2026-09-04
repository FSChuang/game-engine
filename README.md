# Game Engine

A cross-platform 2D game engine built on **SDL3** in **C++17**, developed as a
team over the semester. The engine is the reusable core; each teammate builds
their own game on top of it.

- **Architecture & folder layout:** see [`PROJECT_STRUCTURE.md`](./PROJECT_STRUCTURE.md)
- **Engineering rules (binding):** see [`ENGINEERING_SPEC.md`](./ENGINEERING_SPEC.md)

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
- SDL3_image — needed once texture loading is implemented
