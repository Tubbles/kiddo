add_library(unity STATIC vendor/unity/unity.c)
target_include_directories(unity PUBLIC vendor/unity)

add_executable(kiddo_tests test_stub.c)
target_link_libraries(kiddo_tests PRIVATE unity)
