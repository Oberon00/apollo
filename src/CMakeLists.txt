set(cpplua_HDRS_PUBLIC
    "converters.hpp" "function.hpp"
    "error.hpp"
    "config.hpp"
    "gc.hpp")
set(cpplua_HDRS_DETAIL "detail/function_template.hpp")
source_group("Internal Headers" FILES ${cpplua_HDRS_DETAIL})
set(cpplua_HDRS ${cpplua_HDRS_PUBLIC} ${cpplua_HDRS_DETAIL})
set(cpplua_SRCS "main.cpp")

include_directories(${Boost_INCLUDE_DIRS} ${LUA_INCLUDE_DIR})
add_executable(cpp-lua ${cpplua_HDRS} ${cpplua_SRCS})
target_link_libraries(cpp-lua ${BOOST_LIBRARIES} ${LUA_LIBRARIES})

install(TARGETS cpp-lua RUNTIME DESTINATION bin)
