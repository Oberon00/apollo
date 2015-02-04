#ifndef APOLLO_LUA_INCLUDE_HPP_INCLUDED
#define APOLLO_LUA_INCLUDE_HPP_INCLUDED

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#if LUA_VERSION_NUM < 502
#include <apollo/config.hpp>

#    define lua_rawlen lua_objlen
#    define lua_pushglobaltable(L) lua_pushvalue(L, LUA_GLOBALSINDEX)
#    define LUA_OK 0

namespace apollo { namespace detail {

inline bool is_relative_index(int idx)
{
    return idx < 0 && idx > LUA_REGISTRYINDEX;
}

} } // namespace apollo::detail

inline int lua_absindex(lua_State* L, int idx)
{
    return apollo::detail::is_relative_index(idx) ?
        lua_gettop(L) + idx + 1 : idx;
}

inline void lua_rawsetp(lua_State* L, int t, void const* k)
{
    lua_pushlightuserdata(L, const_cast<void*>(k));
    lua_insert(L, -2); // Move key beneath value.
    if (apollo::detail::is_relative_index(t))
        t -= 1; // Adjust for pushed k.
    lua_rawset(L, t);
}

inline void lua_rawgetp(lua_State* L, int t, void const* k)
{
    lua_pushlightuserdata(L, const_cast<void*>(k));
    if (apollo::detail::is_relative_index(t))
        t -= 1; // Adjust for pushed k.
    lua_rawget(L, t);
}

APOLLO_API void luaL_requiref (
    lua_State *L, const char *modname, lua_CFunction openf, int glb);

#endif // LUA_VERSION_NUM < 502

#endif // APOLLO_LUA_INCLUDE_HPP_INCLUDED
