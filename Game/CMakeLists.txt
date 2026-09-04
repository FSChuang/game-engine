# An individual game that runs on the shared engine. Each teammate has their
# own Game target; this one is yours. It may use engine systems freely but must
# not put reusable logic here (that belongs in Engine/).

add_executable(main
    src/main.cpp
)

# Linking Engine also brings in SDL3 transitively (Engine links it PUBLIC).
target_link_libraries(main PRIVATE Engine)

# When you implement texture loading (Task: sprite animation) you'll need
# SDL3_image. Install it (brew install sdl3_image) and uncomment:
# find_package(SDL3_image REQUIRED)
# target_link_libraries(main PRIVATE SDL3_image::SDL3_image)
