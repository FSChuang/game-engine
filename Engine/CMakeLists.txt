# The reusable, game-agnostic engine. Built as a static library so every
# teammate's game links the SAME engine. This physically enforces the
# engine/game boundary from ENGINEERING_SPEC.md §0 & §5: game code cannot
# live in here, only reusable systems.

find_package(SDL3 REQUIRED)

# List engine source files here as you implement each system.
add_library(Engine STATIC
    src/Engine/Collision/Collision.cpp
    src/Engine/Core/Application.cpp
    src/Engine/Entity/Entity.cpp
    src/Engine/Input/InputManager.cpp
    src/Engine/Physics/PhysicsSystem.cpp
    src/Engine/Renderer/Renderer.cpp
    src/Engine/Time/Timeline.cpp
)

# PUBLIC so games that link Engine can `#include "Engine/..."` and get SDL too.
target_include_directories(Engine PUBLIC src)
target_compile_features(Engine PUBLIC cxx_std_17)
target_link_libraries(Engine PUBLIC SDL3::SDL3)

# Networking (Milestone 2 Section 2) is a separate, optional target: it must not
# make the SDL3-only Engine target above (and everything already built on it) fail
# to configure/build on a machine without ZeroMQ installed. cppzmq's own CMake
# config resolves libzmq internally, so finding cppzmq is sufficient here.
find_package(cppzmq QUIET)
if(cppzmq_FOUND)
    add_library(EngineNetwork STATIC
        src/Engine/Network/Socket.cpp
    )
    target_include_directories(EngineNetwork PUBLIC src)
    target_compile_features(EngineNetwork PUBLIC cxx_std_17)
    target_link_libraries(EngineNetwork PUBLIC cppzmq)
else()
    message(STATUS "cppzmq not found - Engine networking (Milestone 2 Section 2) will be skipped. "
                    "Install with: brew install cppzmq (macOS) or an equivalent libzmq3-dev + cppzmq "
                    "header package on Linux.")
endif()
