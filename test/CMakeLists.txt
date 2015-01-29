add_library(testutil STATIC
    testutil.hpp testutil.cpp
    test_prefix.hpp test_suffix.hpp)

target_link_libraries(testutil
    apollo ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

if (BUILD_SHARED_LIBS AND MSVC)
    add_custom_command(TARGET testutil POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:apollo> $<TARGET_FILE_DIR:testutil>
        COMMENT "Copying apollo DLL to test directory...")
endif()

set (TESTS
    create_class
    create_table
    default_argument
    function_converters
    implicit_ctor
    lua_utils
    object_converters
    overloadset
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

add_executable(benchmark "benchmark.cpp")
target_link_libraries(benchmark ${LUA_LIBRARIES} apollo)
