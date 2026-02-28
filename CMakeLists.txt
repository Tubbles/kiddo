cmake_minimum_required(VERSION 3.20)
project(kiddo C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)

file(GLOB ZLOG_SOURCES src/vendor/zlog/*.c)
add_library(zlog STATIC ${ZLOG_SOURCES})
target_include_directories(zlog PUBLIC src/vendor/zlog)
target_link_libraries(zlog PRIVATE pthread)

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
target_link_libraries(kiddo PRIVATE raylib zlog)

add_subdirectory(test)
