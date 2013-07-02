#ifndef CPPLUA_GC_HPP_INCLUDED
#define CPPLUA_GC_HPP_INCLUDED CPPLUA_GC_HPP_INCLUDED

#include <lua.hpp>
#include <boost/assert.hpp>
#include "converters.hpp"

namespace cpplua {

template <typename T>
int gcObject(lua_State* L)
{
#ifndef NDEBUG
    BOOST_ASSERT(lua_type(L, 1) == LuaType<T>::value);
#endif
    static_cast<T*>(lua_touserdata(L, 1))->~T();
    return 0;
}

template <typename T>
T* pushGcObject(lua_State* L, T const& o)
{
    void* uf = lua_newuserdata(L, sizeof(T));
    new(uf) T(o);
    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, &gcObject<T>);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return static_cast<T*>(uf);
}

} // namespace cpplua

#endif // CPPLUA_GC_HPP_INCLUDED