cmake_minimum_required(VERSION 2.8)
project(apollo)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")


if (MSVC)
    add_definitions("/W4" "/MP")
else()
    list(APPEND CMAKE_CXX_FLAGS "-std=c++11")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set (_cfgfile
            "${CMAKE_CURRENT_SOURCE_DIR}/include/apollo/detail/clang_stdlib_config.hpp")
        add_definitions("-DBOOST_STDLIB_CONFIG=\"${_cfgfile}\"")
        add_definitions("-Weverything")
        add_definitions("-Wno-c++98-compat -Wno-c++98-compat-pedantic")
        add_definitions("-Wno-weak-vtables -Wno-padded")
    endif()
endif()
include_directories("src")

set(Boost_USE_STATIC_LIBS     ON)
set(Boost_USE_MULTITHREADED   ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.53 REQUIRED)

add_definitions("-DBOOST_ALL_NO_LIB")

find_package(Lua52 REQUIRED)
include_directories(
    ${BOOOST_INCLUDE_DIRS} ${LUA_INCLUDE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory("src")
