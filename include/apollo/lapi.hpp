#ifndef APOLLO_LAPI_HPP_INCLUDED
#define APOLLO_LAPI_HPP_INCLUDED APOLLO_LAPI_HPP_INCLUDED

#include <lua.hpp>
#include "converters.hpp"

namespace apollo {

void set_error_msg_handler(lua_State* L);
bool push_error_msg_handler(lua_State* L);

void pcall(lua_State* L, int nargs, int nresults, int msgh);
void pcall(lua_State* L, int nargs, int nresults);

template <typename T>
inline void rawget(lua_State* L, int t, T&& k)
{
    if (t < 0)
        t -= 1;
    push(L, std::forward<T>(k));
    lua_rawget(L, t);
}

inline void rawget(lua_State* L, int t, int k)
{
    lua_rawgeti(L, t, k);
}

inline void rawget(lua_State* L, int t, void const* k)
{
    lua_rawgetp(L, t, k);
}


template <typename T>
inline void rawset(lua_State* L, int t, T&& k)
{
    if (t < 0)
        t -= 1;
    push(L, std::forward<T>(k));
    lua_insert(L, -2); // Move key below value
    lua_rawset(L, t);
}

inline void rawset(lua_State* L, int t, int k)
{
    lua_rawseti(L, t, k);
}

inline void rawset(lua_State* L, int t, void const* k)
{
    lua_rawsetp(L, t, k);
}



} // namespace apollo

#endif // APOLLO_LAPI_HPP_INCLUDED
