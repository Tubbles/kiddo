cmake_minimum_required(VERSION 3.20)
project(kiddo C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)

add_executable(kiddo
    src/main.c
    src/shape.c
    src/render.c
    src/input.c
    src/player.c
    src/particle.c
    src/collision.c
    src/audio.c
)
target_link_libraries(kiddo PRIVATE raylib)

add_subdirectory(test)
