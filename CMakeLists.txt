project(cpp-lua)

if (MSVC)
    add_definitions("/W4" "/MP")
else()
    list(APPEND CMAKE_CXX_FLAGS "-std=c++11")
endif()

set (cpplua_HDRS "converters.hpp")

add_executable(cpp-lua main.cpp)

install(TARGETS cpp-lua RUNTIME DESTINATION bin)
