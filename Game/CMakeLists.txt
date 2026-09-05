# An individual game that runs on the shared engine. Each teammate has their
# own Game target; this one is yours. It may use engine systems freely but must
# not put reusable logic here (that belongs in Engine/).

add_executable(main
    src/Game.cpp
    src/main.cpp
)

# Linking Engine also brings in SDL3 transitively (Engine links it PUBLIC).
target_link_libraries(main PRIVATE Engine)
