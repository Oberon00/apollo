add_library(testutil
    testutil.hpp testutil.cpp
    test_prefix.hpp test_suffix.hpp)
target_link_libraries(testutil
    apollo ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

set (TESTS
    class_utility
    create_table
    function_converters
    lua_utils
    object_converters
    reference
    simple_converters
    typeid
)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_definitions("-Wno-global-constructors" "-Wno-exit-time-destructors")
endif()

foreach(test ${TESTS})
    add_executable(test_${test} test_${test}.cpp)
    target_link_libraries(test_${test} testutil)
    set_target_properties(test_${test} PROPERTIES
        FOLDER "Tests"
        COMPILE_DEFINITIONS "BOOST_TEST_MODULE=${test}")
    add_test(NAME ${test} COMMAND test_${test})
endforeach()

