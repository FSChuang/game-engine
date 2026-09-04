# The reusable, game-agnostic engine. Built as a static library so every
# teammate's game links the SAME engine. This physically enforces the
# engine/game boundary from ENGINEERING_SPEC.md §0 & §5: game code cannot
# live in here, only reusable systems.

find_package(SDL3 REQUIRED)

# List engine source files here as you implement each system.
# (CMake needs at least one .cpp; Core.cpp is the current anchor.)
add_library(Engine STATIC
    src/Engine/Core/Core.cpp
    # src/Engine/Entity/Entity.cpp        # Task 2
    # src/Engine/Renderer/Renderer.cpp    # Task 1 / Task 6
    # src/Engine/Input/InputManager.cpp   # Task 4
    # src/Engine/Physics/PhysicsSystem.cpp# Task 3
    # src/Engine/Collision/Collision.cpp  # Task 5
)

# PUBLIC so games that link Engine can `#include "Engine/..."` and get SDL too.
target_include_directories(Engine PUBLIC src)
target_compile_features(Engine PUBLIC cxx_std_17)
target_link_libraries(Engine PUBLIC SDL3::SDL3)
