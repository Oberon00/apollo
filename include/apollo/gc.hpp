// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifndef APOLLO_GC_HPP_INCLUDED
#define APOLLO_GC_HPP_INCLUDED APOLLO_GC_HPP_INCLUDED

#include <apollo/detail/meta_util.hpp>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <apollo/lua_include.hpp>
#include <apollo/config.hpp>

#include <type_traits>

namespace apollo {

template <typename T>
int gc_object(lua_State* L) BOOST_NOEXCEPT
{
    BOOST_ASSERT(lua_type(L, 1) == LUA_TUSERDATA);
    static_cast<T*>(lua_touserdata(L, 1))->~T();
    return 0;
}

template <typename T, typename... Args>
inline detail::remove_cvr<T>*
emplace_bare_udata(lua_State* L, Args&&... ctor_args)
{
    using obj_t = detail::remove_cvr<T>;
    void* uf = lua_newuserdata(L, sizeof(obj_t));
    try {
        return new(uf) obj_t(std::forward<Args>(ctor_args)...);
    } catch (...) {
        lua_pop(L, 1);
        throw;
    }
}

template <typename T>
inline detail::remove_cvr<T>*
push_bare_udata(lua_State* L, T&& o)
{
    return emplace_bare_udata<T>(L, std::forward<T>(o));
}



// Note: __gc will not unset metatable.
// Use for objects that cannot be retrieved from untrusted Lua code only.
template <typename T>
detail::remove_cvr<T>*
push_gc_object(lua_State* L, T&& o)
{
    using obj_t = detail::remove_cvr<T>;
    void* uf = push_bare_udata(L, std::forward<T>(o));
    APOLLO_DETAIL_CONSTCOND_BEGIN
    if (!std::is_trivially_destructible<obj_t>::value) {
    APOLLO_DETAIL_CONSTCOND_END
        lua_createtable(L, 0, 1); // 0 sequence entries, 1 dictionary entry
        lua_pushcfunction(L, &gc_object<obj_t>);
        lua_setfield(L, -2, "__gc");
        lua_setmetatable(L, -2);
    }
    return static_cast<obj_t*>(uf);
}

} // namespace apollo

#endif // APOLLO_GC_HPP_INCLUDED
