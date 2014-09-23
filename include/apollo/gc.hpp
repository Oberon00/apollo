#ifndef APOLLO_GC_HPP_INCLUDED
#define APOLLO_GC_HPP_INCLUDED APOLLO_GC_HPP_INCLUDED

#include <lua.hpp>
#include <boost/assert.hpp>
#include "converters.hpp"

namespace apollo {

template <typename T>
int gc_object(lua_State* L) BOOST_NOEXCEPT
{
    BOOST_ASSERT(lua_type(L, 1) == LUA_TUSERDATA);
    static_cast<T*>(lua_touserdata(L, 1))->~T();
    return 0;
}

template <typename T>
typename detail::remove_qualifiers<T>::type*
push_gc_object(lua_State* L, T&& o)
{
    typedef typename detail::remove_qualifiers<T>::type obj_t;
    void* uf = lua_newuserdata(L, sizeof(obj_t));
    new(uf) obj_t(std::forward<T>(o)); // TODO: Balance stack in case of exceptions.
    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, &gc_object<obj_t>);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return static_cast<obj_t*>(uf);
}

} // namespace apollo

#endif // APOLLO_GC_HPP_INCLUDED
