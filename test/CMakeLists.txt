add_library(unity STATIC vendor/unity/unity.c)
target_include_directories(unity PUBLIC vendor/unity)

add_executable(kiddo_tests
    test_main.c
    test_stub.c
    test_shape.c
    test_player.c
    test_particle.c
    test_collision.c
    test_entity_shape.c
    test_entity_car.c
    ${CMAKE_SOURCE_DIR}/src/shape.c
    ${CMAKE_SOURCE_DIR}/src/player.c
    ${CMAKE_SOURCE_DIR}/src/particle.c
    ${CMAKE_SOURCE_DIR}/src/collision.c
    ${CMAKE_SOURCE_DIR}/src/render.c
    ${CMAKE_SOURCE_DIR}/src/entity_shape.c
    ${CMAKE_SOURCE_DIR}/src/entity_car.c
    ${CMAKE_SOURCE_DIR}/src/entity_wall.c
    ${CMAKE_SOURCE_DIR}/src/entity_parking.c
)
target_link_libraries(kiddo_tests PRIVATE unity raylib m)
target_include_directories(kiddo_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
