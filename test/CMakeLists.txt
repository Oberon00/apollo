# Part of the apollo library -- Copyright (c) Christian Neumüller 2015
# This file is subject to the terms of the BSD 2-Clause License.
# See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

add_library(testutil STATIC
    testutil.hpp testutil.cpp
    test_prefix.hpp test_suffix.hpp)

target_link_libraries(testutil
    apollo ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

needs_apollo_dll(testutil)

set (TESTS
    call_by_ref
    create_class
    create_table
    default_argument
    function_converters
    implicit_ctor
    lua_utils
    object_converters
    overloadset
    property
    reference
    simple_converters
    typeid
    ward_ptr
)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
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

add_executable(benchmark "benchmark.cpp")
target_link_libraries(benchmark ${LUA_LIBRARIES} apollo)
