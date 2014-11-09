set(apollo_HDRS_PUBLIC
    "class.hpp"
    "create_class.hpp"
    "config.hpp"
    "converters.hpp"
    "converters_fwd.hpp"
    "create_table.hpp"
    "error.hpp"
    "function.hpp"
    "gc.hpp"
    "implicit_ctor.hpp"
    "lapi.hpp"
    "reference.hpp"
    "smart_ptr.hpp"
    "stack_balance.hpp"
    "typeid.hpp"
)
set(apollo_HDRS_DETAIL
    "class_info.hpp"
    "instance_holder.hpp"
    "integer_seq.hpp"
    "light_key.hpp"
    "meta_util.hpp"
    "ref_binder.hpp"
    "variadic_pass.hpp"
)

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
    "function.cpp"
    "lapi.cpp"
    "reference.cpp"
    "stack_balance.cpp"
    "typeid.cpp"
)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set_source_files_properties("lapi.cpp" PROPERTIES
        COMPILE_FLAGS "-Wno-shadow") # BOOST_SCOPE_EXIT
endif()

add_library(apollo ${apollo_HDRS} ${apollo_SRCS})
target_link_libraries(apollo ${LUA_LIBRARIES})

install(TARGETS apollo
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib)
