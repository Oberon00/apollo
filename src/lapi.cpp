// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#include <apollo/builtin_types.hpp>
#include <apollo/error.hpp>
#include <apollo/lapi.hpp>
#include <apollo/detail/light_key.hpp>

#include <boost/exception/info.hpp>
#include <boost/throw_exception.hpp>


namespace apollo {

static apollo::detail::light_key const msghKey = {};

APOLLO_API void set_error_msg_handler(lua_State* L)
{
    lua_rawsetp(L, LUA_REGISTRYINDEX, &msghKey);
}

APOLLO_API bool push_error_msg_handler(lua_State* L)
{
    lua_rawgetp(L, LUA_REGISTRYINDEX, &msghKey);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return false;
    }
    return true;
}

APOLLO_API void pcall(lua_State* L, int nargs, int nresults, int msgh)
{
    int const r = lua_pcall(L, nargs, nresults, msgh);
    if (r != LUA_OK) {
        auto lua_msg = to(L, -1, std::string("(no error message)"));
        lua_pop(L, 1);
        BOOST_THROW_EXCEPTION(lua_api_error()
                              << errinfo::lua_state(L)
                              << errinfo::lua_msg(lua_msg)
                              << errinfo::lua_error_code(r)
                              << errinfo::msg("lua_pcall() failed"));
    }
}

APOLLO_API void pcall(lua_State* L, int nargs, int nresults)
{
    int const msgh = push_error_msg_handler(L) ? lua_absindex(L, -nargs - 2) : 0;
    if (msgh)
        lua_insert(L, msgh); // move beneath arguments and function

    auto cleanup = [L, msgh]() -> void {
        if (msgh)
            lua_remove(L, msgh);
    };

    try { pcall(L, nargs, nresults, msgh); }
    catch (...) {
        cleanup();
        throw;
    }
    cleanup();
}

APOLLO_API void extend_table(lua_State* L, int t, int with)
{
    extend_table_deep(L, t, with, 1);
}

APOLLO_API void extend_table_deep(lua_State* L,
    int t, int with, unsigned max_depth)
{
    if (max_depth == 0)
        return;

    t = lua_absindex(L, t);
    with = lua_absindex(L, with);
    lua_pushnil(L);
    while (lua_next(L, with)) {
        bool extended = false;
        lua_pushvalue(L, -2); // copy key
        // Stack (A): -3; k, -2: with[k], -1: k
        if (max_depth > 1 && lua_type(L, -2) == LUA_TTABLE) {
            lua_rawget(L, t);
            // Stack (B): -3; k, -2: with[k], -1: t[k]
            if (lua_type(L, -1) == LUA_TTABLE) {
                extend_table_deep(L, -1, -2, max_depth - 1);
                lua_pop(L, 2);
                extended = true;
            } else {
                lua_pop(L, 1);
                lua_pushvalue(L, -2); // copy key again
            }
        }
        if (!extended) {
            // Stack is same as (A)
            lua_insert(L, -2); // move key copy under value
            // -3: k, -2: k, -1: v
            lua_rawset(L, t);
        }
    }
}

} // namespace apollo
