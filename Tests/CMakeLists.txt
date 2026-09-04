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
