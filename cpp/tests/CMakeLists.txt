add_executable(test_core
    unit/test_core.cpp
)

target_link_libraries(test_core PRIVATE libcarb)
add_test(NAME test_core COMMAND test_core)
