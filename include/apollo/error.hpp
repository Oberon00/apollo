#ifndef LUABIND_ERROR_HPP_INCLUDED
#define LUABIND_ERROR_HPP_INCLUDED LUABIND_ERROR_HPP_INCLUDED

#include <boost/exception/exception.hpp>
#include <boost/exception/error_info.hpp>
#include <exception>
#include <lua.hpp>
#include <string>
#include <typeinfo>

namespace apollo {

namespace errinfo {
#define ERRINFO(t, n) using n = boost::error_info<struct tag_##n, t>
ERRINFO(std::string, lua_msg);
ERRINFO(std::string, msg);
ERRINFO(int, stack_index);
ERRINFO(lua_State*, lua_state);
ERRINFO(int, lua_error_code);
ERRINFO(std::type_info, source_typeid);
#undef ERRINFO
} // namespace error_info

struct error: virtual std::exception, boost::exception {};

struct no_ref_error: virtual error {};

struct lua_api_error: virtual error {};

struct conversion_error: virtual error {};
struct to_lua_conversion_error: virtual conversion_error {};
struct to_cpp_conversion_error: virtual conversion_error {};
struct class_conversion_error: virtual to_cpp_conversion_error {};
struct ambiguous_base_error: virtual class_conversion_error {};

} // namespace apollo

#endif // LUABIND_ERROR_HPP_INCLUDED
