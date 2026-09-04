cmake_minimum_required(VERSION 3.16)
project(GameEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# All executables land in build/bin so assets paths are predictable across targets.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# The reusable engine (team-owned) and each game that builds on top of it.
add_subdirectory(Engine)
add_subdirectory(Game)

# Unit tests (ENGINEERING_SPEC.md §10). Toggle with -DBUILD_TESTS=OFF.
option(BUILD_TESTS "Build the engine unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
