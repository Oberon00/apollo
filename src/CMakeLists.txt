set(apollo_HDRS_PUBLIC
    "builtin_types.hpp"
    "class.hpp"
    "config.hpp"
    "converters.hpp"
    "converters_fwd.hpp"
    "create_class.hpp"
    "create_table.hpp"
    "default_argument.hpp"
    "emplace_ctor.hpp"
    "error.hpp"
    "function.hpp"
    "function_primitives.hpp"
    "gc.hpp"
    "implicit_ctor.hpp"
    "lapi.hpp"
    "make_function.hpp"
    "operator.hpp"
    "overload.hpp"
    "property.hpp"
    "raw_function.hpp"
    "reference.hpp"
    "smart_ptr.hpp"
    "stack_balance.hpp"
    "typeid.hpp"
    "ward_ptr.hpp"
)
set(apollo_HDRS_DETAIL
    "class_info.hpp"
    "instance_holder.hpp"
    "integer_seq.hpp"
    "light_key.hpp"
    "lua_state.hpp"
    "meta_util.hpp"
    "ref_binder.hpp"
    "signature.hpp"
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
set(apollo_HDRS
    ${apollo_HDRS_PUBLIC} ${apollo_HDRS_DETAIL} ${APOLLO_BUILDINFO_HPP})
set(apollo_SRCS
    "builtin_types.cpp"
    "class.cpp"
    "class_info.cpp"
    "function.cpp"
    "lapi.cpp"
    "overload.cpp"
    "reference.cpp"
    "stack_balance.cpp"
    "typeid.cpp"
)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set_source_files_properties("lapi.cpp" PROPERTIES
        COMPILE_FLAGS "-Wno-shadow") # BOOST_SCOPE_EXIT
endif()

set(CMAKE_DEBUG_POSTFIX "-d")

add_library(apollo ${apollo_HDRS} ${apollo_SRCS})
set_target_properties(apollo PROPERTIES
    COMPILE_DEFINITIONS APOLLO_BUILDING=1)
target_link_libraries(apollo ${LUA_LIBRARIES})

install(TARGETS apollo
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib)
install(FILES ${apollo_HDRS_PUBLIC}
    DESTINATION include/apollo)
install(FILES ${apollo_HDRS_DETAIL}
    DESTINATION include/apollo/detail)
