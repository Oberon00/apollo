// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifndef APOLLO_RAW_FUNCTION_HPP_INCLUDED
#define APOLLO_RAW_FUNCTION_HPP_INCLUDED

#include <apollo/converters_fwd.hpp>
#include <apollo/error.hpp>

namespace apollo {

struct raw_function {
    /* implicit */ BOOST_CONSTEXPR
    raw_function(lua_CFunction f_) BOOST_NOEXCEPT
        : f(f_) {}

    template <lua_CFunction FVal>
    static BOOST_CONSTEXPR raw_function caught() BOOST_NOEXCEPT
    {
        return static_cast<lua_CFunction>([](lua_State* L) -> int {
            return exceptions_to_lua_errors_L(L, FVal);
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
struct converter<raw_function>: converter_base<raw_function> {
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
