set(apollo_HDRS_PUBLIC
    "builtin_types.hpp"
    "class.hpp"
    "closing_lstate.hpp"
    "config.hpp"
    "converters.hpp"
    "converters_fwd.hpp"
    "create_class.hpp"
    "create_table.hpp"
    "ctor_wrapper.hpp"
    "default_argument.hpp"
    "emplace_ctor.hpp"
    "error.hpp"
    "function.hpp"
    "function_primitives.hpp"
    "gc.hpp"
    "implicit_ctor.hpp"
    "lapi.hpp"
    "lua_include.hpp"
    "make_function.hpp"
    "operator.hpp"
    "overload.hpp"
    "property.hpp"
    "raw_function.hpp"
    "reference.hpp"
    "stack_balance.hpp"
    "to_raw_function.hpp"
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
    "smart_ptr.hpp"
    "variadic_pass.hpp"
)

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
    "lua51compat.cpp"
    "overload.cpp"
    "reference.cpp"
    "stack_balance.cpp"
    "typeid.cpp"
)

set(CMAKE_DEBUG_POSTFIX "-d")

add_library(apollo ${apollo_HDRS} ${apollo_SRCS})
set_target_properties(apollo PROPERTIES
    COMPILE_DEFINITIONS APOLLO_BUILDING=1)
target_link_libraries(apollo ${LUA_LIBRARIES})

install(TARGETS apollo
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
install(FILES ${apollo_HDRS_PUBLIC}
    DESTINATION include/apollo)
install(FILES ${apollo_HDRS_DETAIL}
    DESTINATION include/apollo/detail)
