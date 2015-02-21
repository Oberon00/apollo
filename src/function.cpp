// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#include <apollo/function.hpp>
#include <apollo/detail/light_key.hpp>
#include <apollo/stack_balance.hpp>

static apollo::detail::light_key function_tag = {};

APOLLO_API boost::typeindex::type_info const& apollo::detail::function_type(
    lua_State* L, int idx)
{
    stack_balance b(L);

    if (!lua_getupvalue(L, idx, fn_upval_tag))
        return boost::typeindex::type_id<void>().type_info();
    if (lua_touserdata(L, -1) != function_tag)
        return boost::typeindex::type_id<void>().type_info();
    lua_pop(L, 1); // Necessary to keep idx correct:
    BOOST_VERIFY(lua_getupvalue(L, idx, fn_upval_type));
    return *static_cast<boost::typeindex::type_info const*>(
        lua_touserdata(L, -1));
}

APOLLO_API void apollo::detail::push_function_tag(lua_State* L)
{
    lua_pushlightuserdata(L, function_tag);
}
