# The reusable, game-agnostic engine. Built as a static library so every
# teammate's game links the SAME engine. This physically enforces the
# engine/game boundary from ENGINEERING_SPEC.md §0 & §5: game code cannot
# live in here, only reusable systems.

find_package(SDL3 REQUIRED)

# Pure C++ engine logic with no SDL (or any other platform/transport) dependency: the
# runtime object model (Milestone 3) plus the two systems that only ever operate on it.
# Kept as its own target, separate from the SDL-backed Engine target below, specifically
# so a headless process (e.g. a dedicated multiplayer server) can use GameObject/Physics/
# Collision without being forced to link SDL3 at all — mirrors EngineNetworkCore's own
# reasoning in this same spirit, one folder down.
add_library(EngineCore STATIC
    src/Engine/GameObject/GameObject.cpp
    src/Engine/GameObject/GameObjectRegistry.cpp
    src/Engine/Collision/Collision.cpp
    src/Engine/Physics/PhysicsSystem.cpp
)
target_include_directories(EngineCore PUBLIC src)
target_compile_features(EngineCore PUBLIC cxx_std_17)

# The SDL-backed half of the engine: window/input/rendering and the timeline (Timeline
# reads SDL's monotonic clock for its default anchor). Publicly links EngineCore so games
# that link Engine still get GameObject/Physics/Collision transitively, unchanged from
# before this split.
add_library(Engine STATIC
    src/Engine/Core/Application.cpp
    src/Engine/Input/InputManager.cpp
    src/Engine/Renderer/Renderer.cpp
    src/Engine/Time/Timeline.cpp
)

# PUBLIC so games that link Engine can `#include "Engine/..."` and get SDL too.
target_include_directories(Engine PUBLIC src)
target_compile_features(Engine PUBLIC cxx_std_17)
target_link_libraries(Engine PUBLIC SDL3::SDL3 EngineCore)

# Networking protocol/registry (Milestone 2 Section 2): pure C++ logic with no
# ZeroMQ (or any other transport) dependency, so it builds and unit-tests everywhere
# the Engine target does. Kept as its own target, separate from Engine itself, so the
# folder-per-subsystem convention (Engine/src/Engine/Network/) has one physical home
# regardless of whether the optional ZeroMQ transport below is available.
add_library(EngineNetworkCore STATIC
    src/Engine/Network/Protocol.cpp
    src/Engine/Network/PlayerRegistry.cpp
    src/Engine/Network/ServerDispatch.cpp
    src/Engine/Network/ExchangeProtocol.cpp
)
target_include_directories(EngineNetworkCore PUBLIC src)
target_compile_features(EngineNetworkCore PUBLIC cxx_std_17)

# ZeroMQ transport (Milestone 2 Section 2) is a separate, optional target: it must
# not make the SDL3-only Engine target above (and everything already built on it, nor
# EngineNetworkCore above) fail to configure/build on a machine without ZeroMQ
# installed. cppzmq's own CMake config resolves libzmq internally, so finding cppzmq
# is sufficient here.
find_package(cppzmq QUIET)
if(cppzmq_FOUND)
    add_library(EngineNetwork STATIC
        src/Engine/Network/Socket.cpp
    )
    target_include_directories(EngineNetwork PUBLIC src)
    target_compile_features(EngineNetwork PUBLIC cxx_std_17)
    # PUBLIC: code that links EngineNetwork (the eventual real server/client) gets the
    # protocol/registry types transitively along with the transport.
    target_link_libraries(EngineNetwork PUBLIC cppzmq EngineNetworkCore)
else()
    message(STATUS "cppzmq not found - Engine ZeroMQ transport (Milestone 2 Section 2) will be skipped. "
                    "Install with: brew install cppzmq (macOS) or an equivalent libzmq3-dev + cppzmq "
                    "header package on Linux. Protocol/PlayerRegistry (EngineNetworkCore) are unaffected.")
endif()
