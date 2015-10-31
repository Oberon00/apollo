// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifndef APOLLO_RAW_FUNCTION_HPP_INCLUDED
#define APOLLO_RAW_FUNCTION_HPP_INCLUDED

#include <apollo/converters_fwd.hpp>
#include <apollo/error.hpp>


namespace apollo {

namespace detail {

// It seems that although MSVC can inline function pointers passed as
// template arguments it only does so when they are called directly
// through the template argument. When passing the pointer on to another
// function as an ordinary function argument, MSVC seems unable to recognize
// that the function pointer is fixed at compile-time and refuses to inline.
//
// This helper struct calls FVal directly through the template argument
// and MSVC has no problem with inlining operator() of a function
// object of known type.
template <typename F, F FVal>
struct msvc_inlining_helper {
    template <typename... Args> // Function object or pointer.
    auto operator() (Args&&... args)
        -> decltype(FVal(std::forward<Args>(args)...))
    {
        return FVal(std::forward<Args>(args)...);
    }

    template <typename Cls, typename... Args> // Member function pointer.
    auto operator() (Cls&& instance, Args&&... args)
        -> decltype((instance.*FVal)(std::forward<Args>(args)...))
    {
        return (instance.*FVal)(std::forward<Args>(args)...);
    }
};

} // namespace detail

struct raw_function {
    /* implicit */ BOOST_CONSTEXPR
    raw_function(lua_CFunction f_) BOOST_NOEXCEPT
        : f(f_) {}

    template <lua_CFunction FVal>
    static BOOST_CONSTEXPR raw_function caught() BOOST_NOEXCEPT
    {
        return static_cast<lua_CFunction>([](lua_State* L) -> int {
            return exceptions_to_lua_errors_L(
                L, detail::msvc_inlining_helper<lua_CFunction, FVal>());
        });
    }

    /* implicit */ BOOST_CONSTEXPR
    operator lua_CFunction() const BOOST_NOEXCEPT
    {
        return f;
    }

    lua_CFunction f;
};

template <>
struct converter<raw_function>: converter_base<converter<raw_function>> {
    static int push(lua_State* L, raw_function const& rf)
    {
        lua_pushcfunction(L, rf.f);
        return 1;
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        return lua_iscfunction(L, idx) ? 0 : no_conversion;
    }

    static raw_function to(lua_State* L, int idx)
    {
        return lua_tocfunction(L, idx);
    }
};

} // namespace apollo

#endif // APOLLO_RAW_FUNCTION_HPP_INCLUDED
