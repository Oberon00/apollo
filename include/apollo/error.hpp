// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifndef APOLLO_ERROR_HPP_INCLUDED
#define APOLLO_ERROR_HPP_INCLUDED APOLLO_ERROR_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/exception/error_info.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/exception/exception.hpp>
#include <boost/exception/errinfo_type_info_name.hpp>
#include <apollo/lua_include.hpp>

#include <exception>
#include <string>

namespace apollo {

namespace errinfo {
#define ERRINFO(t, n) using n = boost::error_info<struct tag_##n, t>
ERRINFO(std::string, lua_msg);
ERRINFO(std::string, msg);
ERRINFO(int, stack_index);
ERRINFO(lua_State*, lua_state);
ERRINFO(int, lua_error_code);
ERRINFO(char const*, source_typeid_name);
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

template <typename F, typename... Args>
auto exceptions_to_lua_errors(lua_State* L, F&& f, Args&&... args)
    BOOST_NOEXCEPT -> decltype(f(std::forward<Args>(args)...))
{
    int arg = 0;
    try {
        return f(std::forward<Args>(args)...);
    } catch(to_cpp_conversion_error const& e) {
        // TODO: Check for nullptr.
        arg = *boost::get_error_info<errinfo::stack_index>(e);
        lua_pushfstring(L, "%s [%s -> %s]",
            boost::get_error_info<errinfo::msg>(e)->c_str(),
            luaL_typename(L, arg),
            boost::get_error_info<boost::errinfo_type_info_name>(e)->c_str());
    } catch(std::exception const& e) {
        lua_pushfstring(L, "exception: %s", e.what());
    } catch(...) {
        lua_pushliteral(L, "unknown exception");
    }
    if (arg)
        luaL_argerror(L, arg, lua_tostring(L, -1));
    lua_error(L);
    std::abort(); // Should never be reached.
}

template <typename F, typename... Args>
auto exceptions_to_lua_errors_L(lua_State* L, F&& f, Args&&... args)
    BOOST_NOEXCEPT -> decltype(f(L, std::forward<Args>(args)...))
{
    return exceptions_to_lua_errors(
        L, std::forward<F>(f), L, std::forward<Args>(args)...);
}


} // namespace apollo

#endif // APOLLO_ERROR_HPP_INCLUDED
