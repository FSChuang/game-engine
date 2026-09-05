# Spare Parts

[![Build and Tests](https://github.com/FSChuang/game-engine/actions/workflows/ci.yml/badge.svg)](https://github.com/FSChuang/game-engine/actions/workflows/ci.yml)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![SDL3](https://img.shields.io/badge/SDL-3-green.svg)
![CMake](https://img.shields.io/badge/CMake-3.16%2B-informational.svg)

A cross-platform 2D game engine built on **SDL3** in **C++17**, developed as a
team over the semester. The engine is the reusable core; each teammate builds
their own game on top of it.

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
