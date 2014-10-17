set (TESTS
    create_table
    function_converters
    lua_utils
    object_converters
    simple_converters)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_definitions("-Wno-global-constructors" "-Wno-exit-time-destructors")
endif()

foreach(test ${TESTS})
    add_executable(test_${test} test_${test}.cpp)
    target_link_libraries(test_${test}
        apollo ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
    set_target_properties(test_${test} PROPERTIES
        FOLDER "Tests"
        COMPILE_DEFINITIONS "BOOST_TEST_MODULE=${test}")
    add_test(NAME ${test} COMMAND test_${test})
endforeach()

