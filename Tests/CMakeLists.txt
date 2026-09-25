# Engine unit tests (ENGINEERING_SPEC.md §10). This starter uses a tiny
# assert-based harness so there's zero extra dependency to get started; you can
# swap in Catch2 or GoogleTest via FetchContent later without changing layout.
#
# Add one executable per test file (or group related tests), link Engine, and
# register it with CTest so `ctest` runs them all.

add_executable(CoreTests ExampleTest.cpp)
target_link_libraries(CoreTests PRIVATE Engine)
add_test(NAME CoreTests COMMAND CoreTests)

add_executable(EntityTests EntityTest.cpp)
target_link_libraries(EntityTests PRIVATE Engine)
add_test(NAME EntityTests COMMAND EntityTests)

add_executable(PhysicsSystemTests PhysicsSystemTest.cpp)
target_link_libraries(PhysicsSystemTests PRIVATE Engine)
add_test(NAME PhysicsSystemTests COMMAND PhysicsSystemTests)

add_executable(CollisionTests CollisionTest.cpp)
target_link_libraries(CollisionTests PRIVATE Engine)
add_test(NAME CollisionTests COMMAND CollisionTests)

add_executable(ScalingTests ScalingTest.cpp)
target_link_libraries(ScalingTests PRIVATE Engine)
add_test(NAME ScalingTests COMMAND ScalingTests)

add_executable(TimelineTests TimelineTest.cpp)
target_link_libraries(TimelineTests PRIVATE Engine)
add_test(NAME TimelineTests COMMAND TimelineTests)

# Pure protocol/registry logic (Milestone 2 Section 2): no ZeroMQ dependency, so
# these always build and run regardless of whether cppzmq was found.
add_executable(NetworkProtocolTests NetworkProtocolTest.cpp)
target_link_libraries(NetworkProtocolTests PRIVATE EngineNetworkCore)
add_test(NAME NetworkProtocolTests COMMAND NetworkProtocolTests)

add_executable(PlayerRegistryTests PlayerRegistryTest.cpp)
target_link_libraries(PlayerRegistryTests PRIVATE EngineNetworkCore)
add_test(NAME PlayerRegistryTests COMMAND PlayerRegistryTests)

# Headless smoke executables proving Engine::Socket's REQ/REP halves work end to
# end (Milestone 2 Section 2). Manually run, not CTest-registered (see
# NetworkSmoke/SmokeServer.cpp / SmokeClient.cpp for why). Built only when
# EngineNetwork exists, i.e. only when cppzmq was found.
if(TARGET EngineNetwork)
    add_executable(NetworkSmokeServer NetworkSmoke/SmokeServer.cpp)
    target_link_libraries(NetworkSmokeServer PRIVATE EngineNetwork)

    add_executable(NetworkSmokeClient NetworkSmoke/SmokeClient.cpp)
    target_link_libraries(NetworkSmokeClient PRIVATE EngineNetwork)
endif()
