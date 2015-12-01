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
#include <apollo/detail/meta_util.hpp>
#include <apollo/config.hpp>

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

template <typename ErrInfo>
boost::exception& err_supplement(boost::exception& e, ErrInfo&& info)
{
    using info_t = detail::remove_cvr<ErrInfo>;
    if (!boost::get_error_info<info_t>(e))
        e << std::forward<ErrInfo>(info);
    return e;
}

struct error: virtual std::exception, boost::exception {};

struct no_ref_error: virtual error {};

struct lua_api_error: virtual error {};

struct conversion_error: virtual error {};
struct to_lua_conversion_error: virtual conversion_error {};
struct to_cpp_conversion_error: virtual conversion_error {};
struct class_conversion_error: virtual to_cpp_conversion_error {};
struct ambiguous_base_error: virtual class_conversion_error {};

namespace detail {
APOLLO_API int push_current_exception_string(lua_State* L) BOOST_NOEXCEPT;
APOLLO_API BOOST_NORETURN void error_from_pushed_exception_string(
    lua_State* L, int arg) BOOST_NOEXCEPT;
} // namespace detail

template <typename F, typename... Args>
auto exceptions_to_lua_errors(lua_State* L, F&& f, Args&&... args)
    BOOST_NOEXCEPT -> decltype(f(std::forward<Args>(args)...))
{
    int arg = 0;
    try {
        return f(std::forward<Args>(args)...);
    } catch(...) {
        arg = detail::push_current_exception_string(L);
    }
    detail::error_from_pushed_exception_string(L, arg);
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
