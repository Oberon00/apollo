set(apollo_HDRS_PUBLIC
    "converters.hpp" "function.hpp" "class.hpp" "smart_ptr.hpp"
    "error.hpp"
    "config.hpp"
    "gc.hpp"
    "lapi.hpp"
    "reference.hpp"
    "stack_balance.hpp")
set(apollo_HDRS_DETAIL
    "function_template.hpp"
    "instance_holder.hpp"
    "class_info.hpp"
    "light_key.hpp"
    "for_each_type.hpp"
    "base_traits.hpp")

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
    "converters.cpp"
    "reference.cpp"
    "lapi.cpp"
    "class_info.cpp"
    "class.cpp"
    "stack_balance.cpp")

add_library(apollo ${apollo_HDRS} ${apollo_SRCS})
target_link_libraries(apollo ${LUA_LIBRARIES})

install(TARGETS apollo
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib)
