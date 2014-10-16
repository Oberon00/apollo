set(apollo_HDRS_PUBLIC
    "class.hpp"
    "config.hpp"
    "converters.hpp"
    "error.hpp"
    "function.hpp"
    "gc.hpp"
    "lapi.hpp"
    "reference.hpp"
    "smart_ptr.hpp"
    "stack_balance.hpp")
set(apollo_HDRS_DETAIL
    "class_info.hpp"
    "instance_holder.hpp"
    "light_key.hpp")

# http://stackoverflow.com/a/13104057/2128694
macro(prepend_to_all _srcs _path)
    unset(_tmp)
    foreach(src ${${_srcs}})
        list(APPEND _tmp ${_path}${src})
    endforeach()
    set(${_srcs} ${_tmp})
endmacro()

prepend_to_all(apollo_HDRS_PUBLIC "../include/apollo/")
prepend_to_all(apollo_HDRS_DETAIL "../include/apollo/detail/")

source_group("Internal Headers" FILES ${apollo_HDRS_DETAIL})
set(apollo_HDRS ${apollo_HDRS_PUBLIC} ${apollo_HDRS_DETAIL})
set(apollo_SRCS
    "class.cpp"
    "class_info.cpp"
    "converters.cpp"
    "lapi.cpp"
    "reference.cpp"
    "stack_balance.cpp")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set_source_files_properties("lapi.cpp" PROPERTIES
        COMPILE_FLAGS "-Wno-shadow") # BOOST_SCOPE_EXIT
endif()

add_library(apollo ${apollo_HDRS} ${apollo_SRCS})
target_link_libraries(apollo ${LUA_LIBRARIES})

install(TARGETS apollo
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib)
