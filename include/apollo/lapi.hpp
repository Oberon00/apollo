#ifndef APOLLO_LAPI_HPP_INCLUDED
#define APOLLO_LAPI_HPP_INCLUDED APOLLO_LAPI_HPP_INCLUDED

#include <apollo/converters.hpp>
#include <apollo/detail/light_key.hpp>

#include <lua.hpp>

namespace apollo {

void set_error_msg_handler(lua_State* L);
bool push_error_msg_handler(lua_State* L);

void pcall(lua_State* L, int nargs, int nresults, int msgh);
void pcall(lua_State* L, int nargs, int nresults);

template <typename T>
inline void rawget(lua_State* L, int t, T&& k)
{
    t = lua_absindex(L, t);
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

inline void rawget(lua_State* L, int t, void* k)
{
    lua_rawgetp(L, t, k);
}


inline void rawget(lua_State* L, int t, detail::light_key const& k)
{
    lua_rawgetp(L, t, k);
}

inline void rawget(lua_State* L, int t, detail::light_key& k)
{
    lua_rawgetp(L, t, k);
}


template <typename T>
inline void rawset(lua_State* L, int t, T&& k)
{
    t = lua_absindex(L, t);
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

inline void rawset(lua_State* L, int t, void* k)
{
    lua_rawsetp(L, t, k);
}

inline void rawset(lua_State* L, int t, detail::light_key const& k)
{
    lua_rawsetp(L, t, k);
}

inline void rawset(lua_State* L, int t, detail::light_key& k)
{
    lua_rawsetp(L, t, k);
}



} // namespace apollo

#endif // APOLLO_LAPI_HPP_INCLUDED
