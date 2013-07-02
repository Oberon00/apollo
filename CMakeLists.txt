cmake_minimum_required(VERSION 2.8)
project(cpp-lua)

if (MSVC)
    add_definitions("/W4" "/MP")
else()
    list(APPEND CMAKE_CXX_FLAGS "-std=c++11")
endif()

find_package(Boost 1.53 REQUIRED)
find_package(Lua52 REQUIRED)
include_directories(${BOOOST_INCLUDE_DIRS} ${LUA_INCLUDE_DIRS})

add_subdirectory("src")
